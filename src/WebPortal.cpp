#include <Preferences.h>

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "WebPortal.h"
#include "config.h"

Preferences preferences;
const char *PREFERENCES_NAMESPACE = "patro_config";
const char *MACRO_KEY = "macro_seq";

DNSServer dnsServer;
WebServer server(80);
std::vector<MacroStep> currentSequence = {{1, 200}, {2, 200}};

// Função para salvar a sequência
void save_sequence_to_flash(const std::vector<MacroStep> &seq)
{
    String seqString = "";
    for (const auto &step : seq)
    {
        seqString += String(step.button) + "," + String(step.duration) + ";";
    }
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putString(MACRO_KEY, seqString);
    preferences.end();
}

// Função para carregar a sequência
void load_sequence_from_flash()
{
    currentSequence.clear();
    preferences.begin(PREFERENCES_NAMESPACE, true);
    String seqString = preferences.getString(MACRO_KEY, "1,200;2,200;");
    preferences.end();

    int last_idx = 0;
    for (int i = 0; i < seqString.length(); i++)
    {
        if (seqString.charAt(i) == ';')
        {
            String stepStr = seqString.substring(last_idx, i);
            int commaIdx = stepStr.indexOf(',');
            if (commaIdx != -1)
            {
                int button = stepStr.substring(0, commaIdx).toInt();
                int duration = stepStr.substring(commaIdx + 1).toInt();
                currentSequence.push_back({button, duration});
            }
            last_idx = i + 1;
        }
    }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<title>Patro Config</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style> body { font-family: sans-serif; } </style>
</head><body><h1>Configurar Macro</h1>
<p>Formato: Botao,Duracao;Botao,Duracao;...</p>
<p>Ex: 1,200;2,300; (Aperta B1 por 200ms, depois B2 por 300ms)</p>
<form action="/save" method="POST">
<input type="text" name="seq" size="40" value=""><br><br>
<button type="submit">Salvar e Reiniciar</button></form></body></html>)rawliteral";

void web_init()
{
    // A carga agora é feita no setup() principal, mas deixamos a função aqui.
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    dnsServer.start(53, "*", WiFi.softAPIP());

    server.on("/", []()
              { server.send(200, "text/html", index_html); });
    server.onNotFound([]()
                      { server.send(200, "text/html", index_html); });

    server.on("/save", []()
              {
        if (server.hasArg("seq")) {
            // Apenas para validação e armazenamento temporário
            std::vector<MacroStep> newSequence;
            String seqString = server.arg("seq");
            int last_idx = 0;
            for (int i = 0; i < seqString.length(); i++) {
                if (seqString.charAt(i) == ';') {
                    String stepStr = seqString.substring(last_idx, i);
                    int comma_idx = stepStr.indexOf(',');
                    if (comma_idx != -1) {
                        int button = stepStr.substring(0, comma_idx).toInt();
                        int duration = stepStr.substring(comma_idx + 1).toInt();
                        newSequence.push_back({button, duration});
                    }
                    last_idx = i + 1;
                }
            }
            // Se a nova sequência for válida, salva na flash
            if (!newSequence.empty()) {
                currentSequence = newSequence;
                save_sequence_to_flash(currentSequence);
            }
        }
        server.send(200, "text/html", "<h1>Salvo! Reiniciando em 3s...</h1>");
        delay(3000);
        ESP.restart(); });
    server.begin();
}

void web_stop()
{
    server.stop();
    dnsServer.stop();
    WiFi.mode(WIFI_OFF);
}
void web_loop()
{
    dnsServer.processNextRequest();
    server.handleClient();
}
std::vector<MacroStep> web_get_sequence() { return currentSequence; }

void web_load_default_sequence()
{
    load_sequence_from_flash();
}