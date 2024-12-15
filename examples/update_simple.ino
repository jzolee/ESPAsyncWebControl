#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWebControl.h>

#define SSID "your ap ssid"
#define PASSWORD "your ap password"

AsyncWebServer server(80);
AsyncWebUpdate webUpdate;

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    webUpdate.begin(&server); //webUpdate.begin(&server, "/update", "admin", "password");
    server.begin();
}

void loop() {}