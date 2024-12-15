#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWebControl.h>

AsyncWebServer server(80);
AsyncWebConfig webConfig;

AsyncWebConfigCfg config = {
    { "ssid", "your ap ssid" },
    { "password", "your ap password" },
    { "example 1", "example value 1" },
};

void setup() {
    Serial.begin(115200);
    WiFi.begin(config["ssid"], config["password"]);
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    config["param 1"] = "value 1";
    config["param 2"] = "value 2";

    webConfig.begin(&config, &server, "/config", "admin", "password");
    webConfig.onCmd([](const String cmd) {
        Serial.println("CMD: " + cmd);
        webConfig.msg("CMD: " + cmd);
        });

    server.begin();
}

void loop() {}