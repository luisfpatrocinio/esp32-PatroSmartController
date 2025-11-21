#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h> // Keep this for NVS access
#include "WebPortal.h"
#include "config.h"

// --- Global objects ---
DNSServer dnsServer;
WebServer server(80);
std::vector<MacroStep> currentSequence; // Macro definition (managed by WebPortal, used by JoystickController)

// --- Preferences instances ---
Preferences macroPreferences; // For macro storage
Preferences wifiPreferences;  // For Wi-Fi STA credentials storage

// --- Wi-Fi Station Variables ---
String saved_ssid = "";
String saved_password = "";
bool is_ap_mode_active = false; // Tracks if ESP's AP is active
bool is_sta_connected = false;  // Tracks if ESP is successfully connected to an external Wi-Fi station

// --- HELPER FUNCTIONS FOR NVS (Preferences) ---

// Saves the macro sequence to NVS
void save_sequence_to_flash(const std::vector<MacroStep> &seq)
{
    String seqString = "";
    for (const auto &step : seq)
    {
        seqString += String(step.button) + "," + String(step.duration) + ";";
    }
    macroPreferences.begin(PREFERENCES_NAMESPACE_GENERAL, false); // false = read-write
    macroPreferences.putString(MACRO_KEY, seqString);
    macroPreferences.end();
}

// Loads the macro sequence from NVS
void load_sequence_from_flash()
{
    currentSequence.clear();
    macroPreferences.begin(PREFERENCES_NAMESPACE_GENERAL, true);              // true = read-only
    String seqString = macroPreferences.getString(MACRO_KEY, "1,200;2,200;"); // Default macro if none saved
    macroPreferences.end();

    int last_idx = 0;
    for (int i = 0; i < seqString.length(); i++)
    {
        if (seqString.charAt(i) == ';')
        {
            String stepStr = seqString.substring(last_idx, i);
            int comma_idx = stepStr.indexOf(',');
            if (comma_idx != -1)
            {
                int button = stepStr.substring(0, comma_idx).toInt();
                int duration = stepStr.substring(comma_idx + 1).toInt();
                currentSequence.push_back({button, duration});
            }
            last_idx = i + 1;
        }
    }
}

// Loads saved STA Wi-Fi credentials from NVS
void load_station_credentials()
{
    wifiPreferences.begin(PREFERENCES_NAMESPACE_WIFI, true); // read-only
    saved_ssid = wifiPreferences.getString(WIFI_SSID_KEY, "");
    saved_password = wifiPreferences.getString(WIFI_PASS_KEY, "");
    wifiPreferences.end();
    Serial.printf("Loaded Wi-Fi: SSID='%s', Pass='%s'\n", saved_ssid.c_str(), saved_password.c_str());
}

// Saves STA Wi-Fi credentials to NVS
void save_station_credentials(const String &ssid, const String &password)
{
    wifiPreferences.begin(PREFERENCES_NAMESPACE_WIFI, false); // read-write
    wifiPreferences.putString(WIFI_SSID_KEY, ssid);
    wifiPreferences.putString(WIFI_PASS_KEY, password);
    wifiPreferences.end();
    saved_ssid = ssid;
    saved_password = password;
    Serial.printf("Saved Wi-Fi: SSID='%s', Pass='%s'\n", saved_ssid.c_str(), saved_password.c_str());
}

// --- HTML/CSS/JS for Macro Configuration Page (index_html) ---
// This is the updated version with a link to the Wi-Fi config page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>PatroSmart Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        :root { --bg-color: #2c2f33; --primary-color: #7289da; --text-color: #ffffff; --card-bg: #36393f; --border-color: #40444b; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: var(--bg-color); color: var(--text-color); margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; }
        .container { max-width: 600px; width: 100%; }
        h1, h2 { color: var(--primary-color); text-align: center; }
        .card { background-color: var(--card-bg); border-radius: 8px; padding: 20px; margin-bottom: 20px; border: 1px solid var(--border-color); }
        .controls { display: grid; grid-template-columns: repeat(auto-fit, minmax(100px, 1fr)); gap: 10px; margin-bottom: 20px; }
        .btn { background-color: var(--primary-color); color: white; border: none; padding: 12px; border-radius: 5px; font-size: 16px; cursor: pointer; transition: background-color 0.2s; }
        .btn:hover { background-color: #677bc4; }
        .btn:disabled { background-color: #555; cursor: not-allowed; }
        .duration-control { display: flex; flex-direction: column; gap: 10px; margin-top: 20px; }
        .duration-control label { font-weight: bold; }
        .slider-container { display: flex; align-items: center; gap: 15px; }
        input[type=range] { flex-grow: 1; accent-color: var(--primary-color); }
        input[type=number] { width: 80px; padding: 8px; border-radius: 5px; border: 1px solid var(--border-color); background-color: var(--bg-color); color: var(--text-color); font-size: 16px; }
        #sequence-list { list-style: none; padding: 0; }
        .sequence-item { display: flex; justify-content: space-between; align-items: center; background-color: var(--border-color); padding: 10px; border-radius: 5px; margin-bottom: 8px; }
        .sequence-item span { font-size: 1.1em; }
        .remove-btn { background-color: #f04747; color: white; border: none; border-radius: 5px; padding: 5px 10px; cursor: pointer; }
        .main-form button { width: 100%; padding: 15px; font-size: 18px; font-weight: bold; }
        .nav-link { display: block; text-align: center; margin-top: 15px; color: var(--primary-color); text-decoration: none; font-size: 1.1em; }
        .nav-link:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>PatroSmart Controller</h1>

        <div class="card">
            <h2>1. Create Step</h2>
            <div class="controls" id="button-selector">
                <!-- Buttons are generated by JS -->
            </div>
            <div class="duration-control">
                <label for="duration">Step Duration (ms):</label>
                <div class="slider-container">
                    <input type="range" id="duration-slider" min="50" max="2000" value="200" step="10">
                    <input type="number" id="duration-input" min="50" max="5000" value="200">
                </div>
            </div>
            <button class="btn" id="add-step-btn" style="width:100%; margin-top: 20px;">Add Step to Macro</button>
        </div>

        <div class="card">
            <h2>2. Current Macro</h2>
            <div id="sequence-list"></div>
            <p id="empty-macro-msg">The macro is empty. Add steps above.</p>
        </div>
        
        <form id="save-form" class="main-form">
            <button class="btn" type="submit">Save Macro & Restart</button>
        </form>

        <a href="/wifi" class="nav-link">Configure Wi-Fi Connection</a>
    </div>

    <script>
        // --- GLOBAL STATE ---
        let macroSequence = [];
        let selectedButton = 1;

        // --- DOM ELEMENT REFERENCES ---
        const buttonSelector = document.getElementById('button-selector');
        const durationSlider = document.getElementById('duration-slider');
        const durationInput = document.getElementById('duration-input');
        const addStepBtn = document.getElementById('add-step-btn');
        const sequenceList = document.getElementById('sequence-list');
        const emptyMacroMsg = document.getElementById('empty-macro-msg');
        const saveForm = document.getElementById('save-form');

        // --- EVENT LISTENERS & INITIALIZATION ---

        // Sync duration slider and number input
        durationSlider.addEventListener('input', (e) => durationInput.value = e.target.value);
        durationInput.addEventListener('input', (e) => durationSlider.value = e.target.value);

        // Generate button selectors
        for (let i = 1; i <= 8; i++) {
            const btn = document.createElement('button');
            btn.className = 'btn';
            btn.textContent = `Button ${i}`;
            btn.dataset.buttonId = i;
            if (i === 1) btn.style.backgroundColor = '#4CAF50'; // Highlight the default selected button
            btn.addEventListener('click', () => {
                selectedButton = i;
                document.querySelectorAll('#button-selector .btn').forEach(b => b.style.backgroundColor = 'var(--primary-color)');
                btn.style.backgroundColor = '#4CAF50';
            });
            buttonSelector.appendChild(btn);
        }

        // Add step button listener
        addStepBtn.addEventListener('click', () => {
            const duration = parseInt(durationInput.value, 10);
            if (duration < 50) {
                alert("Duration must be at least 50ms.");
                return;
            }
            macroSequence.push({ button: selectedButton, duration: duration });
            renderSequence();
        });

        // Remove step from macro (using event delegation for performance)
        sequenceList.addEventListener('click', (e) => {
            if (e.target.classList.contains('remove-btn')) {
                const index = parseInt(e.target.dataset.index, 10);
                macroSequence.splice(index, 1);
                renderSequence();
            }
        });
        
        // Handle form submission using AJAX (fetch)
        saveForm.addEventListener('submit', (e) => {
            e.preventDefault(); // Prevent the default form submission which causes a page reload

            // Prepare the data string for the ESP32
            const formattedString = macroSequence.map(step => `${step.button},${step.duration}`).join(';');
            const payload = formattedString.length > 0 ? formattedString + ';' : '';
            
            // Provide user feedback
            const btn = saveForm.querySelector('button');
            const originalText = btn.textContent;
            btn.textContent = "Saving...";
            btn.disabled = true;

            const formData = new FormData();
            formData.append("seq", payload);

            // Send the data to the server
            fetch('/save', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    // On success, replace the page content with a success message
                    document.querySelector('.container').innerHTML = `
                        <h1>PatroSmart Controller</h1>
                        <div class="card" style="text-align: center;">
                            <h2 style="color: #4CAF50;">Success!</h2>
                            <p style="font-size: 1.2em;">Configuration saved.</p>
                            <p>The device is restarting and will enter Bluetooth mode.</p>
                            <p style="color: #aaa; font-size: 0.9em;">You can now close this page.</p>
                        </div>
                    `;
                } else {
                    alert("Error: Could not save macro.");
                    btn.textContent = originalText;
                    btn.disabled = false;
                }
            })
            .catch(error => {
                // A network error is expected here because the ESP32 restarts immediately after
                // sending the response, which can terminate the connection abruptly.
                // We can often assume success if an error occurs after a short period.
                console.error('Fetch error (likely due to device restart):', error);
                 // We still show the success message because the data was likely received.
                 document.querySelector('.container').innerHTML = `
                        <h1>PatroSmart Controller</h1>
                        <div class="card" style="text-align: center;">
                            <h2 style="color: #4CAF50;">Success!</h2>
                            <p style="font-size: 1.2em;">Configuration sent.</p>
                            <p>The device is restarting and will enter Bluetooth mode.</p>
                            <p style="color: #aaa; font-size: 0.9em;">You can now close this page.</p>
                        </div>
                    `;
            });
        });


        // --- UI RENDERING ---
        function renderSequence() {
            sequenceList.innerHTML = '';
            emptyMacroMsg.style.display = macroSequence.length === 0 ? 'block' : 'none';

            macroSequence.forEach((step, index) => {
                const item = document.createElement('div');
                item.className = 'sequence-item';
                item.innerHTML = `
                    <span>Press <b>Button ${step.button}</b> for <b>${step.duration}ms</b></span>
                    <button class="remove-btn" data-index="${index}">X</button>
                `;
                sequenceList.appendChild(item);
            });
        }

        // --- INITIAL DATA LOAD ---
        // Fetch the currently saved macro when the page loads
        document.addEventListener('DOMContentLoaded', () => {
            fetch('/get_macro')
                .then(response => response.text())
                .then(data => {
                    if (data && data.length > 0) {
                        const steps = data.split(';').filter(step => step); // filter removes empty strings
                        macroSequence = steps.map(step => {
                            const parts = step.split(',');
                            return { button: parseInt(parts[0], 10), duration: parseInt(parts[1], 10) };
                        });
                        renderSequence(); // Update the UI with the loaded macro
                    }
                })
                .catch(error => console.error('Error loading initial macro:', error));
        });
    </script>
</body>
</html>
)rawliteral";

// --- NEW HTML/CSS/JS for Wi-Fi Configuration Page (wifi_config_html) ---
const char wifi_config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Wi-Fi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        :root { --bg-color: #2c2f33; --primary-color: #7289da; --text-color: #ffffff; --card-bg: #36393f; --border-color: #40444b; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background-color: var(--bg-color); color: var(--text-color); margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; }
        .container { max-width: 600px; width: 100%; }
        h1, h2 { color: var(--primary-color); text-align: center; }
        .card { background-color: var(--card-bg); border-radius: 8px; padding: 20px; margin-bottom: 20px; border: 1px solid var(--border-color); }
        .btn { background-color: var(--primary-color); color: white; border: none; padding: 12px; border-radius: 5px; font-size: 16px; cursor: pointer; transition: background-color 0.2s; }
        .btn:hover { background-color: #677bc4; }
        .btn:disabled { background-color: #555; cursor: not-allowed; }
        .input-group { margin-bottom: 15px; }
        .input-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .input-group input[type="text"], .input-group input[type="password"], .input-group select { 
            width: calc(100% - 22px); padding: 10px; border-radius: 5px; border: 1px solid var(--border-color); 
            background-color: var(--bg-color); color: var(--text-color); font-size: 16px; 
        }
        .status-message { margin-top: 15px; padding: 10px; border-radius: 5px; text-align: center; }
        .status-success { background-color: #4CAF50; color: white; }
        .status-error { background-color: #f04747; color: white; }
        .status-info { background-color: #7289da; color: white; }
        .loading-spinner { border: 4px solid rgba(255,255,255,0.3); border-top: 4px solid var(--primary-color); border-radius: 50%; width: 24px; height: 24px; animation: spin 1s linear infinite; display: inline-block; vertical-align: middle; margin-right: 10px;}
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
        .nav-link { display: block; text-align: center; margin-top: 15px; color: var(--primary-color); text-decoration: none; font-size: 1.1em; }
        .nav-link:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Wi-Fi Connection Setup</h1>
        <div class="card">
            <h2>Select Network</h2>
            <div class="input-group">
                <label for="ssid-select">Available Networks:</label>
                <select id="ssid-select" class="form-control">
                    <option value="">-- Select SSID --</option>
                </select>
            </div>
            <button id="scan-btn" class="btn" style="width:100%; margin-top: 10px;">Scan for Networks</button>
            <div id="scan-status" class="status-message status-info" style="display:none;"></div>
        </div>

        <div class="card">
            <h2>Network Credentials</h2>
            <div class="input-group">
                <label for="ssid-input">SSID (or manual entry):</label>
                <input type="text" id="ssid-input" placeholder="Enter network name">
            </div>
            <div class="input-group">
                <label for="password-input">Password:</label>
                <input type="password" id="password-input" placeholder="Enter password">
            </div>
            <button id="connect-btn" class="btn" style="width:100%; margin-top: 20px;">Connect & Save</button>
            <div id="connect-status" class="status-message" style="display:none;"></div>
        </div>

        <a href="/" class="nav-link">Back to Macro Configuration</a>
    </div>

    <script>
        // --- DOM ELEMENT REFERENCES ---
        const ssidSelect = document.getElementById('ssid-select');
        const ssidInput = document.getElementById('ssid-input');
        const passwordInput = document.getElementById('password-input');
        const scanBtn = document.getElementById('scan-btn');
        const connectBtn = document.getElementById('connect-btn');
        const scanStatus = document.getElementById('scan-status');
        const connectStatus = document.getElementById('connect-status');

        // --- EVENT LISTENERS ---

        // Sync SSID input with selected dropdown option
        ssidSelect.addEventListener('change', () => {
            ssidInput.value = ssidSelect.value;
        });

        // Scan for networks
        scanBtn.addEventListener('click', () => {
            scanStatus.style.display = 'block';
            scanStatus.className = 'status-message status-info';
            scanStatus.innerHTML = '<span class="loading-spinner"></span> Scanning...';
            scanBtn.disabled = true;

            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    ssidSelect.innerHTML = '<option value="">-- Select SSID --</option>'; // Clear existing options
                    if (networks.length === 0) {
                        scanStatus.innerHTML = 'No networks found.';
                    } else {
                        networks.forEach(network => {
                            const option = document.createElement('option');
                            option.value = network.ssid;
                            option.textContent = `${network.ssid} (${network.rssi} dBm)`; // Show RSSI for signal strength
                            ssidSelect.appendChild(option);
                        });
                        scanStatus.innerHTML = `Found ${networks.length} networks.`;
                    }
                    scanBtn.disabled = false;
                })
                .catch(error => {
                    console.error('Error scanning networks:', error);
                    scanStatus.className = 'status-message status-error';
                    scanStatus.innerHTML = 'Error scanning networks.';
                    scanBtn.disabled = false;
                });
        });

        // Connect to selected/entered network
        connectBtn.addEventListener('click', () => {
            const ssid = ssidInput.value;
            const password = passwordInput.value;

            if (!ssid) {
                connectStatus.style.display = 'block';
                connectStatus.className = 'status-message status-error';
                connectStatus.innerHTML = 'Please enter an SSID.';
                return;
            }

            connectStatus.style.display = 'block';
            connectStatus.className = 'status-message status-info';
            connectStatus.innerHTML = '<span class="loading-spinner"></span> Connecting...';
            connectBtn.disabled = true;
            scanBtn.disabled = true; // Disable scan during connection attempt

            const formData = new FormData();
            formData.append('ssid', ssid);
            formData.append('password', password);

            fetch('/set_wifi', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                if (data === 'OK') {
                    // On success, replace the page content with a success message
                    document.querySelector('.container').innerHTML = `
                        <h1>Wi-Fi Connection Setup</h1>
                        <div class="card" style="text-align: center;">
                            <h2 style="color: #4CAF50;">Success!</h2>
                            <p style="font-size: 1.2em;">Wi-Fi credentials saved.</p>
                            <p>The device is restarting and attempting to connect to <b>${ssid}</b>.</p>
                            <p style="color: #aaa; font-size: 0.9em;">You can now close this page.</p>
                        </div>
                    `;
                } else {
                    connectStatus.className = 'status-message status-error';
                    connectStatus.innerHTML = `Connection failed: ${data}`;
                    connectBtn.disabled = false;
                    scanBtn.disabled = false;
                }
            })
            .catch(error => {
                 // A network error is expected here because the ESP32 restarts immediately after
                // sending the response, which can terminate the connection abruptly.
                // We can often assume success if an error occurs after a short period.
                console.error('Fetch error (likely due to device restart):', error);
                 document.querySelector('.container').innerHTML = `
                        <h1>Wi-Fi Connection Setup</h1>
                        <div class="card" style="text-align: center;">
                            <h2 style="color: #4CAF50;">Success!</h2>
                            <p style="font-size: 1.2em;">Wi-Fi credentials sent.</p>
                            <p>The device is restarting and attempting to connect to <b>${ssid}</b>.</p>
                            <p style="color: #aaa; font-size: 0.9em;">You can now close this page.</p>
                        </div>
                    `;
            });
        });

        // --- INITIAL LOAD (Optional: could load saved SSID here if needed) ---
        // For now, we rely on the user scanning or manually entering.
    </script>
</body>
</html>
)rawliteral";

// --- FUNÇÃO PRIVADA para registrar todas as rotas do servidor ---
void register_server_handlers()
{
    // Macro Config Endpoints
    server.on("/", HTTP_GET, []()
              { server.send(200, "text/html", index_html); });
    server.on("/get_macro", HTTP_GET, []()
              {
        String seqString = "";
        for (const auto& step : currentSequence) {
            seqString += String(step.button) + "," + String(step.duration) + ";";
        }
        server.send(200, "text/plain", seqString); });
    server.on("/save", HTTP_POST, []()
              {
                  if (server.hasArg("seq"))
                  {
                      std::vector<MacroStep> newSequence;
                      String seqString = server.arg("seq");
                      int last_idx = 0;
                      for (int i = 0; i < seqString.length(); i++)
                      {
                          if (seqString.charAt(i) == ';')
                          {
                              String stepStr = seqString.substring(last_idx, i);
                              int comma_idx = stepStr.indexOf(',');
                              if (comma_idx != -1)
                              {
                                  int button = stepStr.substring(0, comma_idx).toInt();
                                  int duration = stepStr.substring(comma_idx + 1).toInt();
                                  newSequence.push_back({button, duration});
                              }
                              last_idx = i + 1;
                          }
                      }
                      if (!newSequence.empty())
                      {
                          currentSequence = newSequence;
                          save_sequence_to_flash(currentSequence);
                      }
                  }
                  server.send(200, "text/plain", "OK");
                  delay(100);
                  ESP.restart(); // Restart after saving macro
              });

    // Wi-Fi Config Endpoints
    server.on("/wifi", HTTP_GET, []()
              { server.send(200, "text/html", wifi_config_html); });
    server.on("/scan", HTTP_GET, []()
              {
        WiFi.mode(WIFI_AP_STA);
        Serial.println("Scanning Wi-Fi networks in AP_STA mode...");

        int n = WiFi.scanNetworks(); 
        String json = "[";
        for (int i = 0; i < n; ++i) {
            // Filtra redes sem SSID (geralmente redes ocultas ou inválidas)
            if (WiFi.SSID(i).length() > 0) {
                if (json.length() > 1) {
                    json += ",";
                }
                json += "{";
                json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
                json += ",\"rssi\":" + String(WiFi.RSSI(i));
                json += "}";
            }
        }
        json += "]";
        
        WiFi.scanDelete(); // Clear results to free memory

        server.send(200, "application/json", json); });
    server.on("/set_wifi", HTTP_POST, []()
              {
                  String ssid = server.arg("ssid");
                  String password = server.arg("password");

                  Serial.printf("Attempting to save & connect to Wi-Fi: %s\n", ssid.c_str());

                  // Save credentials immediately
                  save_station_credentials(ssid, password);

                  // Send OK before restart
                  server.send(200, "text/plain", "OK");
                  delay(100);    // Give client a moment to receive response
                  ESP.restart(); // Restart the ESP to try connecting to the new STA
              });

    server.onNotFound([]()
                      { server.send(200, "text/html", index_html); }); // Default fallback to macro config
}

// --- CORE WEB SERVER INITIALIZATION AND LOOP ---

void web_init()
{
    Serial.println("Starting Wi-Fi Access Point and Web Server for configuration.");
    WiFi.mode(WIFI_AP_STA); // Use AP_STA para permitir o scan sem desconectar
    WiFi.softAP(AP_SSID, AP_PASS);

    dnsServer.start(53, "*", WiFi.softAPIP());
    register_server_handlers(); // Registra as rotas
    server.begin();             // Inicia o servidor

    is_ap_mode_active = true;
    Serial.printf("AP SSID: %s | IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void web_server_start()
{
    register_server_handlers(); // Apenas registra as rotas
    server.begin();             // E inicia o servidor
    Serial.println("Web server started in STA mode.");
}

void web_loop()
{ // Esta é para o modo AP
    dnsServer.processNextRequest();
    server.handleClient();
}

// NOVA FUNÇÃO para o loop do servidor no modo STA
void web_server_loop()
{
    server.handleClient(); // Apenas processa clientes HTTP
}

void web_stop()
{
    if (is_ap_mode_active)
    {
        server.stop();
        dnsServer.stop();
        WiFi.mode(WIFI_OFF); // Turn off Wi-Fi completely when stopping AP
        is_ap_mode_active = false;
        Serial.println("Stopped Wi-Fi Access Point and Web Server.");
    }
    // Also disconnect from STA if connected
    if (is_sta_connected)
    {
        WiFi.disconnect(true, true); // Disconnect and clear existing credentials from esp
        is_sta_connected = false;
        Serial.println("Disconnected from Wi-Fi Station.");
    }
}

std::vector<MacroStep> web_get_sequence() { return currentSequence; }
void web_load_default_sequence() { load_sequence_from_flash(); }

// --- NEW Wi-Fi Management Function Implementations ---

// Attempts to load saved credentials and connect to a station Wi-Fi network.
void web_init_sta_mode()
{
    load_station_credentials(); // Try to load saved credentials
    if (saved_ssid.length() > 0)
    {
        Serial.printf("Trying to connect to saved Wi-Fi: %s\n", saved_ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(saved_ssid.c_str(), saved_password.c_str());

        // Do not block in setup(). Let loop() manage this.
        // We'll just check status later.
    }
    else
    {
        Serial.println("No saved Wi-Fi credentials found.");
    }
}

// Disconnects from station mode.
void web_stop_sta_mode()
{
    if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA)
    {                                // Check if STA is part of current mode
        WiFi.disconnect(true, true); // Disconnect and clear credentials from esp
        is_sta_connected = false;
        Serial.println("Disconnected from STA mode.");
    }
}

// Helper to check if the ESP32's Access Point is currently active.
bool web_is_in_ap_mode() { return is_ap_mode_active; }

// Helper to check if the ESP32 is currently connected to an external Wi-Fi station.
bool web_is_in_sta_mode()
{
    // This needs to be actively checked, not just a flag set once.
    // The connection status can change.
    is_sta_connected = (WiFi.status() == WL_CONNECTED);
    return is_sta_connected;
}
