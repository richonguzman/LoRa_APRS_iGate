#include <ArduinoJson.h>
#include "configuration.h"
#include "ota_utils.h"
#include "web_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration               Config;
extern uint32_t                    lastBeaconTx;
extern std::vector<ReceivedPacket> receivedPackets;

extern const char web_index_html[] asm("_binary_data_embed_index_html_gz_start");
extern const char web_index_html_end[] asm("_binary_data_embed_index_html_gz_end");
extern const size_t web_index_html_len = web_index_html_end - web_index_html;

extern const char web_style_css[] asm("_binary_data_embed_style_css_gz_start");
extern const char web_style_css_end[] asm("_binary_data_embed_style_css_gz_end");
extern const size_t web_style_css_len = web_style_css_end - web_style_css;

extern const char web_script_js[] asm("_binary_data_embed_script_js_gz_start");
extern const char web_script_js_end[] asm("_binary_data_embed_script_js_gz_end");
extern const size_t web_script_js_len = web_script_js_end - web_script_js;

extern const char web_bootstrap_css[] asm("_binary_data_embed_bootstrap_css_gz_start");
extern const char web_bootstrap_css_end[] asm("_binary_data_embed_bootstrap_css_gz_end");
extern const size_t web_bootstrap_css_len = web_bootstrap_css_end - web_bootstrap_css;

extern const char web_bootstrap_js[] asm("_binary_data_embed_bootstrap_js_gz_start");
extern const char web_bootstrap_js_end[] asm("_binary_data_embed_bootstrap_js_gz_end");
extern const size_t web_bootstrap_js_len = web_bootstrap_js_end - web_bootstrap_js;

// Declare external symbols for the embedded image data
extern const unsigned char favicon_data[] asm("_binary_data_embed_favicon_png_gz_start");
extern const unsigned char favicon_data_end[] asm("_binary_data_embed_favicon_png_gz_end");
extern const size_t favicon_data_len = favicon_data_end - favicon_data;


namespace WEB_Utils {

    AsyncWebServer server(80);

    void handleNotFound(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", "Not found");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    }

    void handleStatus(AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "OK");
    }

    void handleHome(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (const uint8_t*)web_index_html, web_index_html_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleFavicon(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "image/x-icon", (const uint8_t*)favicon_data, favicon_data_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleReadConfiguration(AsyncWebServerRequest *request) {
        if(Config.webadmin.active && !request->authenticate(Config.webadmin.username.c_str(), Config.webadmin.password.c_str()))
            return request->requestAuthentication();

        File file = SPIFFS.open("/igate_conf.json");
        
        String fileContent;
        while(file.available()){
            fileContent += String((char)file.read());
        }

        request->send(200, "application/json", fileContent);
    }

    void handleReceivedPackets(AsyncWebServerRequest *request) {
        StaticJsonDocument<1536> data;

        for (int i = 0; i < receivedPackets.size(); i++) {
            data[i]["rxTime"]   = receivedPackets[i].rxTime;
            data[i]["packet"]   = receivedPackets[i].packet;
            data[i]["RSSI"]     = receivedPackets[i].RSSI;
            data[i]["SNR"]      = receivedPackets[i].SNR;
        }

        String buffer;

        serializeJson(data, buffer);

        request->send(200, "application/json", buffer);
    }

    void handleWriteConfiguration(AsyncWebServerRequest *request) {
        Serial.println("Got new config from www");

        int networks = request->getParam("wifi.APs", true)->value().toInt();

        Config.wifiAPs = {};

        for (int i=0; i<networks; i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = request->getParam("wifi.AP." + String(i) + ".ssid", true)->value();
            wifiap.password               = request->getParam("wifi.AP." + String(i) + ".password", true)->value();

            Config.wifiAPs.push_back(wifiap);
        }

        Config.callsign                 = request->getParam("callsign", true)->value();

        Config.wifiAutoAP.password      = request->getParam("wifi.autoAP.password", true)->value();
        Config.wifiAutoAP.timeout       = request->getParam("wifi.autoAP.timeout", true)->value().toInt();
        

        Config.aprs_is.active           = request->hasParam("aprs_is.active", true);
        Config.aprs_is.passcode         = request->getParam("aprs_is.passcode", true)->value();
        Config.aprs_is.server           = request->getParam("aprs_is.server", true)->value();
        Config.aprs_is.port             = request->getParam("aprs_is.port", true)->value().toInt();
        Config.aprs_is.filter           = request->getParam("aprs_is.filter", true)->value();
        Config.aprs_is.messagesToRF     = request->hasParam("aprs_is.messagesToRF", true);
        Config.aprs_is.objectsToRF      = request->hasParam("aprs_is.objectsToRF", true);


        Config.beacon.interval              = request->getParam("beacon.interval", true)->value().toInt();
        Config.beacon.sendViaAPRSIS         = request->hasParam("beacon.sendViaAPRSIS", true);
        Config.beacon.sendViaRF             = request->hasParam("beacon.sendViaRF", true);
        Config.beacon.latitude              = request->getParam("beacon.latitude", true)->value().toDouble();
        Config.beacon.longitude             = request->getParam("beacon.longitude", true)->value().toDouble();
        Config.beacon.comment               = request->getParam("beacon.comment", true)->value();
        Config.beacon.overlay               = request->getParam("beacon.overlay", true)->value();
        Config.beacon.symbol                = request->getParam("beacon.symbol", true)->value();
        Config.beacon.path                  = request->getParam("beacon.path", true)->value();

        Config.beacon.gpsActive             = request->hasParam("beacon.gpsActive", true);
        Config.beacon.gpsAmbiguity          = request->hasParam("beacon.gpsAmbiguity", true);


        Config.digi.mode                    = request->getParam("digi.mode", true)->value().toInt();
        Config.digi.ecoMode                 = request->hasParam("digi.ecoMode", true);


        Config.loramodule.txFreq            = request->getParam("lora.txFreq", true)->value().toInt();
        Config.loramodule.rxFreq            = request->getParam("lora.rxFreq", true)->value().toInt();
        Config.loramodule.spreadingFactor   = request->getParam("lora.spreadingFactor", true)->value().toInt();
        Config.loramodule.signalBandwidth   = request->getParam("lora.signalBandwidth", true)->value().toInt();
        Config.loramodule.codingRate4       = request->getParam("lora.codingRate4", true)->value().toInt();
        Config.loramodule.power             = request->getParam("lora.power", true)->value().toInt();
        Config.loramodule.txActive          = request->hasParam("lora.txActive", true);
        Config.loramodule.rxActive          = request->hasParam("lora.rxActive", true);


        Config.display.alwaysOn                 = request->hasParam("display.alwaysOn", true);
        if (!Config.display.alwaysOn) {
            Config.display.timeout  = request->getParam("display.timeout", true)->value().toInt();
        }
        Config.display.turn180                  = request->hasParam("display.turn180", true);


        Config.battery.sendInternalVoltage      = request->hasParam("battery.sendInternalVoltage", true);
        Config.battery.monitorInternalVoltage   = request->hasParam("battery.monitorInternalVoltage", true);
        Config.battery.internalSleepVoltage     = request->getParam("battery.internalSleepVoltage", true)->value().toFloat();

        Config.battery.sendExternalVoltage      = request->hasParam("battery.sendExternalVoltage", true);
        if (Config.battery.sendExternalVoltage) {
            Config.battery.externalVoltagePin   = request->getParam("battery.externalVoltagePin", true)->value().toInt();
            Config.battery.voltageDividerR1     = request->getParam("battery.voltageDividerR1", true)->value().toFloat();
            Config.battery.voltageDividerR2     = request->getParam("battery.voltageDividerR2", true)->value().toFloat();
        }
        Config.battery.monitorExternalVoltage   = request->hasParam("battery.monitorExternalVoltage", true);
        Config.battery.externalSleepVoltage     = request->getParam("battery.externalSleepVoltage", true)->value().toFloat();

        Config.battery.sendVoltageAsTelemetry   = request->hasParam("battery.sendVoltageAsTelemetry", true);
        
        Config.wxsensor.active                  = request->hasParam("wxsensor.active", true);
        Config.wxsensor.heightCorrection        = request->getParam("wxsensor.heightCorrection", true)->value().toInt();
        Config.wxsensor.temperatureCorrection   = request->getParam("wxsensor.temperatureCorrection", true)->value().toFloat();
        if (Config.wxsensor.active) {
            Config.beacon.symbol = "_";
        }

        Config.syslog.active                = request->hasParam("syslog.active", true);
        if (Config.syslog.active) {
            Config.syslog.server            = request->getParam("syslog.server", true)->value();
            Config.syslog.port              = request->getParam("syslog.port", true)->value().toInt();
        }

        Config.tnc.enableServer             = request->hasParam("tnc.enableServer", true);
        Config.tnc.enableSerial             = request->hasParam("tnc.enableSerial", true);
        Config.tnc.acceptOwn                = request->hasParam("tnc.acceptOwn", true);

        Config.rebootMode                   = request->hasParam("other.rebootMode", true);
        Config.rebootModeTime               = request->getParam("other.rebootModeTime", true)->value().toInt();

        Config.ota.username                 = request->getParam("ota.username", true)->value();
        Config.ota.password                 = request->getParam("ota.password", true)->value();

        Config.rememberStationTime          = request->getParam("other.rememberStationTime", true)->value().toInt();

        Config.backupDigiMode               = request->hasParam("other.backupDigiMode", true);
        Config.lowPowerMode                 = request->hasParam("other.lowPowerMode", true);
        Config.lowVoltageCutOff             = request->getParam("other.lowVoltageCutOff", true)->value().toDouble();

        Config.personalNote                 = request->getParam("personalNote", true)->value();

        Config.blackList                    = request->getParam("blackList", true)->value();

        Config.webadmin.active              = request->hasParam("webadmin.active", true);
        if (Config.webadmin.active) {
            Config.webadmin.username        = request->getParam("webadmin.username", true)->value();
            Config.webadmin.password        = request->getParam("webadmin.password", true)->value();
        }

        Config.ntp.gmtCorrection            = request->getParam("ntp.gmtCorrection", true)->value().toFloat();

        Config.writeFile();

        AsyncWebServerResponse *response    = request->beginResponse(302, "text/html", "");
        response->addHeader("Location", "/");
        request->send(response);
        displayToggle(false);
        ESP.restart();
    }

    void handleAction(AsyncWebServerRequest *request) {
        String type = request->getParam("type", false)->value();

        if (type == "send-beacon") {
            lastBeaconTx = 0;

            request->send(200, "text/plain", "Beacon will be sent in a while");
        } else if (type == "reboot") {
            displayToggle(false);
            ESP.restart();
        } else {
            request->send(404, "text/plain", "Not Found");
        }
    }

    void handleStyle(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/css", (const uint8_t*)web_style_css, web_style_css_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleScript(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/javascript", (const uint8_t*)web_script_js, web_script_js_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }

    void handleBootstrapStyle(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/css", (const uint8_t*)web_bootstrap_css, web_bootstrap_css_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    }

    void handleBootstrapScript(AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/javascript", (const uint8_t*)web_bootstrap_js, web_bootstrap_js_len);
        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Cache-Control", "max-age=3600");
        request->send(response);
    }

    void setup() {
        if (!Config.digi.ecoMode) {
            server.on("/", HTTP_GET, handleHome);
            server.on("/status", HTTP_GET, handleStatus);
            server.on("/received-packets.json", HTTP_GET, handleReceivedPackets);
            server.on("/configuration.json", HTTP_GET, handleReadConfiguration);
            server.on("/configuration.json", HTTP_POST, handleWriteConfiguration);
            server.on("/action", HTTP_POST, handleAction);
            server.on("/style.css", HTTP_GET, handleStyle);
            server.on("/script.js", HTTP_GET, handleScript);
            server.on("/bootstrap.css", HTTP_GET, handleBootstrapStyle);
            server.on("/bootstrap.js", HTTP_GET, handleBootstrapScript);
            server.on("/favicon.png", HTTP_GET, handleFavicon);

            OTA_Utils::setup(&server); // Include OTA Updater for WebServer

            server.onNotFound(handleNotFound);

            server.begin();
        }
    }

}