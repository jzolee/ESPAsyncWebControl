#pragma once

#include <Arduino.h>

#include <unordered_map>
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

using AsyncWebConfigCmdHandler = std::function<void(const String&)>;
using AsyncWebConfigCfg = std::unordered_map<String, String>;

class AsyncWebConfig {
public:
    void begin(AsyncWebConfigCfg* config, AsyncWebServer* server, const char* url = "/config", const char* username = "", const char* password = "");
    void onCmd(AsyncWebConfigCmdHandler cb) { _cmd_cb = cb; }
    void msg(const char* message) { _socket->printfAll("msg#%s", message); }
    void msg(const String& message) { msg(message.c_str()); }

private:
    void _handle_config_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client);
    void _send_config_elements(AsyncWebSocketClient* client);
    void _send_value(String& id, String& value) { _socket->printfAll("value#%s#%s", id.c_str(), value.c_str()); }

    AsyncWebServer* _server;
    AsyncWebSocket* _socket;

    AsyncWebConfigCmdHandler _cmd_cb = NULL;

    String _username = "";
    String _password = "";
    bool _authRequired = false;

    AsyncWebConfigCfg* _config;
};
