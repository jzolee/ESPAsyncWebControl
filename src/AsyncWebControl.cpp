#include "AsyncWebControl.h"

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
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", control_min_html_gz, control_min_html_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
        });

    String wsUrl = url;
    wsUrl += "ws";

    _socket = new AsyncWebSocket(wsUrl);

    if (_authRequired)
        _socket->setAuthentication(_username.c_str(), _password.c_str());

    _socket->onEvent([&](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) -> void {
        if (type == WS_EVT_CONNECT)
            _send_control_elements(client);
        else if (type == WS_EVT_DATA)
            _handle_control_message(arg, data, len, client);

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
    return String();
}

void AsyncWebControl::_handle_control_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client)
{
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::vector<String> m = str_split((char*)data, len, '#');
        if (m.size() == 2)
            for (element& element : _elements)
                if (element.id == m[0]) {
                    element.value = m[1];
                    if (element.fn != NULL)
                        element.fn(element.value);
                    break;
                }
    }
}

void AsyncWebControl::_send_control_elements(AsyncWebSocketClient* client) {
    String str;
    if (_title.length()) {
        char buf[64];
        snprintf(buf, sizeof(buf), "title#%s\n", _title.c_str());
        str += buf;
    }

    for (element& element : _elements) {
        char buf[256];
        const char* type_str;
        switch (element.type) {
        case TEXT: type_str = "text"; break;
        case RANGE: type_str = "range"; break;
        case CHECKBOX: type_str = "checkbox"; break;
        case CBUTTON: type_str = "button"; break;
        default: type_str = "undefined";
        }
        snprintf(buf, sizeof(buf), "add#%s#%s#%s#%.2f#%.2f#%.2f\n",
            type_str, element.id.c_str(), element.value.c_str(), element.min, element.max, element.step);
        str += buf;
    }
    client->text(str);
}

void AsyncWebControl::_send_value(const char* id, const char* value)
{
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "value#%s#%s\n", id, value);
    _socket->textAll(buf, len);
}
