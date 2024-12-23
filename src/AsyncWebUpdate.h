
#pragma once

#include <Arduino.h>

#if defined(ESP32)
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #include <Update.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
    #include <Updater.h>
#endif

#include <ESPAsyncWebServer.h>

#include "control.min.html.gz.h"

class AsyncWebUpdate {

public:
    void begin(AsyncWebServer* server, const char* url = "/update", const char* username = "", const char* password = "");

private:
    void _restart();

    AsyncWebServer* _server;
    AsyncWebSocket* _socket;
    String _username = "";
    String _password = "";
    bool _authRequired = false;
};
