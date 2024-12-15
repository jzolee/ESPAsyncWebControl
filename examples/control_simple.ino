#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWebControl.h>

#define SSID "your ap ssid"
#define PASSWORD "your ap password"

AsyncWebServer server(80);
AsyncWebControl webControl;

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    webControl.addTitle("Device Control");
    webControl.addText("text 1", "Initial text", [](const String value) {
        Serial.println("text 1 = " + value);
        webControl.msg("text 1 = " + value);
        });
    webControl.addRange("range 1", 50, 0, 100, 1, [](const String value) {
        Serial.println("range 1 = " + value);
        webControl.msg("range 1 = " + value);
        });
    webControl.addRange("range 2", 2.5f, -3.14f, 3.14f, 0.01f, [](const String value) {
        Serial.println("range 2 = " + value);
        webControl.msg("range 2 = " + value);
        });
    webControl.addCheckbox("checkbox 1", true, [](const String value) {
        Serial.println("checkbox 1 = " + value);
        webControl.msg("checkbox 1 = " + value);
        });
    webControl.addButton("button 1", "ON", [](const String value) {
        Serial.println("button 1 = " + value);
        webControl.msg("button 1 = " + value);
        webControl.set("button 1", value == "OFF" ? "ON" : "OFF");
        });
    webControl.addButton("reboot button", "Reboot", [](const String value) {
        ESP.restart();
        });

    webControl.begin(&server); //webControl.begin(&server, "/control", "admin", "password");

    server.on("/", HTTP_GET, [&](AsyncWebServerRequest* request) {
        request->redirect("/control");
        });

    server.begin();
}

void loop() {}