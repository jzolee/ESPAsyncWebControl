#pragma once

#include <Arduino.h>

#include <map>
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

typedef std::function<void(String& data)> AsyncWebConfigCmdHandler;
typedef std::map<String, String> AsyncWebConfigCfg;

class AsyncWebConfig {
public:
    void begin(AsyncWebConfigCfg* config, AsyncWebServer* server, const char* url = "/config", const char* username = "", const char* password = "");
    void onCmd(AsyncWebConfigCmdHandler cb) { _cmd_cb = cb; }
    void msg(const char* message) { _socket->printfAll("msg#%s\n", message); }
    void msg(const String& message) { msg(message.c_str()); }

private:
    void _handle_config_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client);
    void _send_config_elements(AsyncWebSocketClient* client);
    void _send_value(String& id, String& value) { _socket->printfAll("value#%s#%s\n", id.c_str(), value.c_str()); }

    AsyncWebServer* _server;
    AsyncWebSocket* _socket;

    AsyncWebConfigCmdHandler _cmd_cb = NULL;

    String _username = "";
    String _password = "";
    bool _authRequired = false;

    AsyncWebConfigCfg* _config;
};

void AsyncWebConfig::begin(AsyncWebConfigCfg* config, AsyncWebServer* server, const char* url, const char* username, const char* password)
{
    _config = config;
    _server = server;

    if (strlen(username) > 0) {
        _authRequired = true;
        _username = username;
        _password = password;
    }

    _server->on(url, HTTP_GET, [&](AsyncWebServerRequest* request) {
        if (_authRequired)
            if (!request->authenticate(_username.c_str(), _password.c_str()))
                return request->requestAuthentication();
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", control_min_html_gz, sizeof(control_min_html_gz));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    String wsUrl = url;
    wsUrl += "ws";

    _socket = new AsyncWebSocket(wsUrl);

    if (_authRequired)
        _socket->setAuthentication(_username.c_str(), _password.c_str());

    _socket->onEvent([&](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) -> void {
        switch (type) {
        case WS_EVT_CONNECT:
            _send_config_elements(client);
            break;
        case WS_EVT_DISCONNECT:
            break;
        case WS_EVT_DATA:
            _handle_config_message(arg, data, len, client);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        }
        _socket->cleanupClients();
    });

    _server->addHandler(_socket);
}

void AsyncWebConfig::_handle_config_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client)
{
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::vector<String> m;
        str_split(m, (char*)data, len, '#');
        if (m.size() > 1) {
            if (m[0] == "CMD") {
                if (m[1] == "cfg" && m.size() == 4) {
                    _config->insert_or_assign(m[2], m[3]);
                    client->text("msg#Done\n");
                    _send_value(m[2], m[3]);
                } else {
                    if (_cmd_cb != NULL) {
                        _cmd_cb(m[1]);
                    }
                }
            } else {
                if (m.size() == 2 && (_config->find(m[0]) != _config->end())) {
                    if (m[1].length() > 0) {
                        _config->insert_or_assign(m[0], m[1]);
                        client->text("msg#Done\n");
                        _send_value(m[2], m[3]);
                    } else {
                        _config->erase(m[0]);
                        msg("'" + m[0] + "' delete from config");
                        _socket->printfAll("del#%s\n", m[0].c_str());
                    }
                }
            }
        }
    }
}

void AsyncWebConfig::_send_config_elements(AsyncWebSocketClient* client)
{
    String str = "title#CONFIGURATION\n";
    str += "add#text#CMD#\n";
    for (const auto& element : *_config) {
        str += "add#text#";
        str += element.first.c_str();
        str += "#";
        str += element.second.c_str();
        str += '\n';
    }
    client->text(str);
}