#pragma once

#include <Arduino.h>
#include <Updater.h>

#if defined(ESP32)
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include "control.min.html.gz.h"

class AsyncWebUpdate {

public:
    void begin(AsyncWebServer* server, const char* url = "/update", const char* username = "", const char* password = "")
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

        _server->on(url, HTTP_POST, [&](AsyncWebServerRequest* request) {
        if(_authRequired)
            if(!request->authenticate(_username.c_str(), _password.c_str()))
                return request->requestAuthentication();
        
        AsyncWebServerResponse *response = request->beginResponse((Update.hasError())?500:200, "text/plain", (Update.hasError())?"FAIL":"OK");
        response->addHeader("Connection", "close");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        _restart(); }, [&](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {

        if(_authRequired)
            if(!request->authenticate(_username.c_str(), _password.c_str()))
                return request->requestAuthentication();

        if (!index) {
#if defined(ESP8266)
                Update.runAsync(true);
                uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin(maxSketchSpace, U_FLASH)){
#elif defined(ESP32)
               if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
#endif
                return request->send(400, "text/plain", "OTA could not begin");
            }
        }
        
        if(len)
            if (Update.write(data, len) != len) 
                return request->send(400, "text/plain", "OTA could not begin");        
            
        if (final) { 
            if (!Update.end(true))
                return request->send(400, "text/plain", "Could not end OTA");
        } else
            return; });

        String wsUrl = url;
        wsUrl += "ws";

        _socket = new AsyncWebSocket(wsUrl);

        if (_authRequired)
            _socket->setAuthentication(_username.c_str(), _password.c_str());

        _socket->onEvent([&](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) -> void {
            switch (type) {
            case WS_EVT_CONNECT:
                client->text("title#Firmware OTA Update");
                client->text(F("raw#<form action='?name=firmware' enctype='multipart/form-data' method='POST'><input type='file' accept='.bin,.bin.gz' name='firmware'><input type='submit' value='Update'></form>"));
                break;
            case WS_EVT_DISCONNECT:
            case WS_EVT_DATA:
            case WS_EVT_PONG:
            case WS_EVT_ERROR:
                break;
            }
            _socket->cleanupClients();
        });

        _server->addHandler(_socket);
    }

private:
    void _restart()
    {
        yield();
        delay(1000);
        yield();
        ESP.restart();
    }
    AsyncWebServer* _server;
    AsyncWebSocket* _socket;
    String _username = "";
    String _password = "";
    bool _authRequired = false;
};
