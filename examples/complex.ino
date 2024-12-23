#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <vector>

#include "ESPAsyncWebControl.h"

AsyncWebServer server(80);

AsyncWebConfig webConfig;
AsyncWebControl webControl;
AsyncWebUpdate webUpdate;

ESP8266WiFiMulti wifiMulti;

AsyncWebConfigCfg config = { //default config
    { "local ip", "192.168.0.168" },
    { "gateway", "192.168.0.1" },
    { "subnet", "255.255.0.0" },
    { "dns 1", "8.8.8.8" },
    { "dns 2", "8.8.4.4" },
    { "ap 1", "AP1:password1" },
    { "ap 2", "AP2:password2" },
    { "ap 3", "AP3:password3" },
};

bool config_load()
{
    if (LittleFS.begin()) {
        File file = LittleFS.open("/config.txt", "r");
        if (file) {
            while (file.available()) {
                String name = file.readStringUntil('=');
                String value = file.readStringUntil('\n');
                //config.insert_or_assign(name, value);
                config[name] = value;
            }
            file.close();
        }
        LittleFS.end();
        return true;
    }
    return false;
}

bool config_save()
{
    if (LittleFS.begin()) {
        File file = LittleFS.open("/config.txt", "w");
        if (file) {
            for (const auto& element : config)
                file.printf("%s=%s\n", element.first.c_str(), element.second.c_str());
            file.close();
        }
        LittleFS.end();
        return true;
    }
    return false;
}

bool config_delete()
{
    bool done = false;
    if (LittleFS.begin()) {
        if (LittleFS.remove("/config.txt"))
            done = true;
        LittleFS.end();
    }
    return done;
}

void config_cmd_handler(const String str)
{
    if (str == "save") {
        if (config_save())
            webConfig.msg("save: done!");
    } else if (str == "delete") {
        if (config_delete())
            webConfig.msg("delete: done!");
    } else if (str == "restart") {
        ESP.restart();
    } else if (str == "help") {
        String str = "available commands:<br>";
        str += "help -> this message<br>";
        str += "save -> save config to filesystem<br>";
        str += "delete -> delete config from filesystem<br>";
        str += "restart -> restart the chip<br>";
        str += "cfg#NewConfig#Value -> create 'NewConfig' with 'Value'<br>";
        str += "others:<br>";
        str += "empty existing config value -> that configuration is deleted<br>";
        str += "!! don't use '#' in config value !!<br>";
        webConfig.msg(str);
    }
}

void init_wifi()
{
    WiFi.mode(WIFI_STA);

    IPAddress local_IP, gateway, subnet, primary_DNS, secondary_DNS;

    local_IP.fromString(config["local ip"]);
    gateway.fromString(config["gateway"]);
    subnet.fromString(config["subnet"]);
    primary_DNS.fromString(config["dns 1"]);
    secondary_DNS.fromString(config["dns 2"]);

    WiFi.config(local_IP, gateway, subnet, primary_DNS, secondary_DNS);

    for (auto element : config) {
        if (element.first.startsWith("ap")) {
            std::vector<String> v;
            str_split(v, element.second, ':');
            if (v[0].length() > 0)
                wifiMulti.addAP(v[0].c_str(), v[1].c_str());
        }
    }

    if (wifiMulti.run(10000) != WL_CONNECTED) {
        WiFi.mode(WIFI_AP);
        String apName = WiFi.macAddress();
        apName.replace(":", "");
        WiFi.softAP(apName);
    }
}

void setup()
{
    Serial.begin(115200);

    config_load();

    init_wifi();

    webConfig.onCmd(config_cmd_handler);
    webConfig.begin(&config, &server, "/config", "admin", "password");

    webUpdate.begin(&server, "/update", "admin", "password");

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

void loop()
{
    wifiMulti.run();
    delay(1000);
}
