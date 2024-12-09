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

typedef std::function<void(const String& value)> AsyncWebControlHandler;

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

void AsyncWebControl::begin(AsyncWebServer* server, const char* url, const char* username, const char* password)
{
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
            _send_control_elements(client);
            break;
        case WS_EVT_DISCONNECT:
            break;
        case WS_EVT_DATA:
            _handle_control_message(arg, data, len, client);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        }
        _socket->cleanupClients();
    });

    _server->addHandler(_socket);
}

void AsyncWebControl::set(const char* id, const char* value)
{
    for (element& elem : _elements)
        if (elem.id == id) {
            elem.value = value;
            break;
        }
    _send_value(id, value);
}

String AsyncWebControl::get(const char* id)
{
    for (element& elem : _elements)
        if (elem.id == id)
            return elem.value;
    return "";
}

void AsyncWebControl::_handle_control_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client)
{
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::vector<String> m;
        str_split(m, (char*)data, len, '#');
        if (m.size() == 2)
            for (element& elem : _elements)
                if (elem.id == m[0]) {
                    elem.value = m[1];
                    if (elem.fn != NULL)
                        elem.fn(elem.value);
                    break;
                }
    }
}

void AsyncWebControl::_send_control_elements(AsyncWebSocketClient* client)
{
    String str;
    if (_title.length())
        str = "title#" + _title + '\n';

    for (element& elem : _elements) {
        str += "add#";
        switch (elem.type) {
        case TEXT:
            str += "text#";
            break;
        case RANGE:
            str += "range#";
            break;
        case CHECKBOX:
            str += "checkbox#";
            break;
        case CBUTTON:
            str += "button#";
            break;
        }
        str += elem.id + "#";
        str += elem.value + "#";
        str += String(elem.min, 2) + "#";
        str += String(elem.max, 2) + "#";
        str += String(elem.step, 2) + '\n';
    }
    client->text(str);
}

void AsyncWebControl::_send_value(const char* id, const char* value)
{
    String str = "value#";
    str += id;
    str += "#";
    str += value;
    str += '\n';

    _socket->textAll(str);
}
