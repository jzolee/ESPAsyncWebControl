#pragma once

#include <Arduino.h>

#include <vector>

#if defined(ESP32)
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "str_split.h"
#include "control.min.html.gz.h"

using AsyncWebControlHandler = std::function<void(const String&)>;

class AsyncWebControl {
    enum Type {
        TEXT,
        RANGE,
        CHECKBOX,
        CBUTTON,
    };

public:
    void begin(AsyncWebServer* server, const char* url = "/control", const char* username = "", const char* password = "");
    void addTitle(const char* title) { _title = title; }
    void addText(const char* id, const char* value, AsyncWebControlHandler callback = NULL) { _elements.push_back({ TEXT, id, value, 0, 0, 0, callback }); }
    void addRange(const char* id, const float value, const float min, const float max, const float step, AsyncWebControlHandler callback = NULL) { _elements.push_back({ RANGE, id, String(value, 2), min, max, step, callback }); }
    void addCheckbox(const char* id, const bool value, AsyncWebControlHandler callback = NULL) { _elements.push_back({ CHECKBOX, id, value ? "1" : "0", 0, 0, 0, callback }); }
    void addButton(const char* id, const char* value, AsyncWebControlHandler callback = NULL) { _elements.push_back({ CBUTTON, id, value, 0, 0, 0, callback }); }
    void set(const char* id, const char* value);
    void set(const char* id, const String& value) { set(id, value.c_str()); }
    String get(const char* id);
    void msg(const char* message) { _socket->printfAll("msg#%s\n", message); }
    void msg(const String& message) { msg(message.c_str()); }

private:
    void _handle_control_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client);
    void _send_control_elements(AsyncWebSocketClient* client);
    void _send_value(const char* id, const char* value);

    AsyncWebServer* _server;
    AsyncWebSocket* _socket;

    String _username = "";
    String _password = "";
    bool _authRequired = false;

    String _title = "";

    struct element {
        Type type;
        String id;
        String value;
        float min;
        float max;
        float step;
        AsyncWebControlHandler fn;
    };

    std::vector<element> _elements;
};
