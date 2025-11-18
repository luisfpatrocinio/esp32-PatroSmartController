#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "WebPortal.h"
#include "config.h"

DNSServer dnsServer;
WebServer server(80);
std::vector<MacroStep> currentSequence = {{1, 200}, {2, 200}}; 

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<title>Patro Config</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
</head><body><h1>Configurar Macro</h1>
<form action="/save" method="POST">
<input type="text" name="seq" placeholder="1,2..."><br>
<input type="number" name="dur" value="100"><br>
<button type="submit">Salvar</button></form></body></html>)rawliteral";

void web_init() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    server.on("/", []() { server.send(200, "text/html", index_html); });
    server.onNotFound([]() { server.send(200, "text/html", index_html); });
    server.on("/save", []() {
        if (server.hasArg("seq")) {
            currentSequence.clear();
            String seq = server.arg("seq");
            int dur = server.hasArg("dur") ? server.arg("dur").toInt() : 100;
            int r = 0;
            for (int i = 0; i < seq.length(); i++) {
                if (seq.charAt(i) == ',') {
                    currentSequence.push_back({seq.substring(r, i).toInt(), dur});
                    r = i + 1;
                }
            }
            if (r < seq.length()) currentSequence.push_back({seq.substring(r).toInt(), dur});
        }
        server.send(200, "text/html", "<h1>Salvo!</h1><a href='/'>Voltar</a>");
    });
    server.begin();
}

void web_stop() { server.stop(); dnsServer.stop(); WiFi.mode(WIFI_OFF); }
void web_loop() { dnsServer.processNextRequest(); server.handleClient(); }
std::vector<MacroStep> web_get_sequence() { return currentSequence; }