#include "AsyncWebConfig.h"

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
        if (type == WS_EVT_CONNECT)
            _send_config_elements(client);
        else if (type == WS_EVT_DATA)
            _handle_config_message(arg, data, len, client);
        _socket->cleanupClients();
        });

    _server->addHandler(_socket);
}

void AsyncWebConfig::_handle_config_message(void* arg, uint8_t* data, size_t len, AsyncWebSocketClient* client)
{
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        std::vector<String> m = str_split((char*)data, len, '#');
        if (m.size() > 1) {
            if (m[0] == "CMD") {
                if (m[1] == "cfg" && m.size() == 4) {
                    //_config->insert_or_assign(m[2], m[3]);
                    (*_config)[m[2]] = m[3];
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
                        //_config->insert_or_assign(m[0], m[1]);
                        (*_config)[m[0]] = m[1];
                        client->text("msg#Done\n");
                        _send_value(m[0], m[1]);
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
    String str =
        "title#CONFIGURATION\n"
        "add#text#CMD#\n";
    for (const auto& element : *_config) {
        char buf[128];
        snprintf(buf, sizeof(buf), "add#text#%s#%s\n", element.first.c_str(), element.second.c_str());
        str += buf;
    }
    client->text(str);
}
