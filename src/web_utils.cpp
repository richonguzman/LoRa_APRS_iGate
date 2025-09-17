/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

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

        auto getParamStringSafe = [&](const String& name, const String& defaultValue = "") -> String {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value();
            }
            return defaultValue;
        };

        auto getParamIntSafe = [&](const String& name, int defaultValue = 0) -> int {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value().toInt();
            }
            return defaultValue;
        };

        auto getParamFloatSafe = [&](const String& name, float defaultValue = 0.0) -> float {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value().toFloat();
            }
            return defaultValue;
        };

        auto getParamDoubleSafe = [&](const String& name, double defaultValue = 0.0) -> double {
            if (request->hasParam(name, true)) {
                return request->getParam(name, true)->value().toDouble();
            }
            return defaultValue;
        };

        int networks = getParamIntSafe("wifi.APs");

        Config.wifiAPs = {};

        for (int i = 0; i < networks; i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = getParamStringSafe("wifi.AP." + String(i) + ".ssid");
            wifiap.password               = getParamStringSafe("wifi.AP." + String(i) + ".password");

            Config.wifiAPs.push_back(wifiap);
        }

        Config.callsign                     = getParamStringSafe("callsign", Config.callsign);
        
        Config.wifiAutoAP.password          = getParamStringSafe("wifi.autoAP.password", Config.wifiAutoAP.password);
        Config.wifiAutoAP.timeout           = getParamIntSafe("wifi.autoAP.timeout", Config.wifiAutoAP.timeout);

        Config.aprs_is.active               = request->hasParam("aprs_is.active", true);
        if (Config.aprs_is.active) {
            Config.aprs_is.messagesToRF     = request->hasParam("aprs_is.messagesToRF", true);
            Config.aprs_is.objectsToRF      = request->hasParam("aprs_is.objectsToRF", true);
            Config.aprs_is.server           = getParamStringSafe("aprs_is.server", Config.aprs_is.server);
            Config.aprs_is.passcode         = getParamStringSafe("aprs_is.passcode", Config.aprs_is.passcode);
            Config.aprs_is.port             = getParamIntSafe("aprs_is.port", Config.aprs_is.port);
            Config.aprs_is.filter           = getParamStringSafe("aprs_is.filter", Config.aprs_is.filter);
        }

        Config.beacon.interval              = getParamIntSafe("beacon.interval", Config.beacon.interval);
        Config.beacon.sendViaAPRSIS         = request->hasParam("beacon.sendViaAPRSIS", true);
        Config.beacon.sendViaRF             = request->hasParam("beacon.sendViaRF", true);
        Config.beacon.latitude              = getParamDoubleSafe("beacon.latitude", Config.beacon.latitude);
        Config.beacon.longitude             = getParamDoubleSafe("beacon.longitude", Config.beacon.longitude);
        Config.beacon.comment               = getParamStringSafe("beacon.comment", Config.beacon.comment);
        Config.beacon.overlay               = getParamStringSafe("beacon.overlay", Config.beacon.overlay);
        Config.beacon.symbol                = getParamStringSafe("beacon.symbol", Config.beacon.symbol);
        Config.beacon.path                  = getParamStringSafe("beacon.path", Config.beacon.path);

        Config.beacon.statusActive          = request->hasParam("beacon.statusActive", true);
        if (Config.beacon.statusActive) {
            Config.beacon.statusPacket      = getParamStringSafe("beacon.statusPacket", Config.beacon.statusPacket);
        }

        Config.beacon.gpsActive             = request->hasParam("beacon.gpsActive", true);
        Config.beacon.gpsAmbiguity          = request->hasParam("beacon.gpsAmbiguity", true);

        Config.personalNote                 = getParamStringSafe("personalNote", Config.personalNote);

        Config.blacklist                    = getParamStringSafe("blacklist", Config.blacklist);

        Config.digi.mode                    = getParamIntSafe("digi.mode", Config.digi.mode);
        Config.digi.ecoMode                 = getParamIntSafe("digi.ecoMode", Config.digi.ecoMode);

        Config.loramodule.txFreq            = getParamIntSafe("lora.txFreq", Config.loramodule.txFreq);
        Config.loramodule.rxFreq            = getParamIntSafe("lora.rxFreq", Config.loramodule.rxFreq);
        Config.loramodule.spreadingFactor   = getParamIntSafe("lora.spreadingFactor", Config.loramodule.spreadingFactor);
        Config.loramodule.signalBandwidth   = getParamIntSafe("lora.signalBandwidth", Config.loramodule.signalBandwidth);
        Config.loramodule.codingRate4       = getParamIntSafe("lora.codingRate4", Config.loramodule.codingRate4);
        Config.loramodule.power             = getParamIntSafe("lora.power", Config.loramodule.power);
        Config.loramodule.txActive          = request->hasParam("lora.txActive", true);
        Config.loramodule.rxActive          = request->hasParam("lora.rxActive", true);


        Config.display.alwaysOn             = request->hasParam("display.alwaysOn", true);
        if (!Config.display.alwaysOn) {
            Config.display.timeout          = getParamIntSafe("display.timeout", Config.display.timeout);
        }
        Config.display.turn180              = request->hasParam("display.turn180", true);


        Config.battery.sendInternalVoltage          = request->hasParam("battery.sendInternalVoltage", true);
        Config.battery.monitorInternalVoltage       = request->hasParam("battery.monitorInternalVoltage", true);
        if (Config.battery.monitorInternalVoltage) {
            Config.battery.internalSleepVoltage     = getParamFloatSafe("battery.internalSleepVoltage", Config.battery.internalSleepVoltage);
        }

        Config.battery.sendExternalVoltage          = request->hasParam("battery.sendExternalVoltage", true);
        if (Config.battery.sendExternalVoltage) {
            Config.battery.externalVoltagePin       = getParamIntSafe("battery.externalVoltagePin", Config.battery.externalVoltagePin);
            Config.battery.voltageDividerR1         = getParamFloatSafe("battery.voltageDividerR1", Config.battery.voltageDividerR1);
            Config.battery.voltageDividerR2         = getParamFloatSafe("battery.voltageDividerR2", Config.battery.voltageDividerR2);
        }
        Config.battery.monitorExternalVoltage       = request->hasParam("battery.monitorExternalVoltage", true);
        if (Config.battery.monitorExternalVoltage) {
            Config.battery.externalSleepVoltage     = getParamFloatSafe("battery.externalSleepVoltage", Config.battery.externalSleepVoltage);
        }
        Config.battery.sendVoltageAsTelemetry       = request->hasParam("battery.sendVoltageAsTelemetry", true);


        Config.wxsensor.active                      = request->hasParam("wxsensor.active", true);
        if (Config.wxsensor.active) {
            Config.wxsensor.heightCorrection        = getParamIntSafe("wxsensor.heightCorrection", Config.wxsensor.heightCorrection);
            Config.wxsensor.temperatureCorrection   = getParamFloatSafe("wxsensor.temperatureCorrection", Config.wxsensor.temperatureCorrection);
            Config.beacon.symbol = "_";
        }


        Config.syslog.active                    = request->hasParam("syslog.active", true);
        if (Config.syslog.active) {
            Config.syslog.server                = getParamStringSafe("syslog.server", Config.syslog.server);
            Config.syslog.port                  = getParamIntSafe("syslog.port", Config.syslog.port);
            Config.syslog.logBeaconOverTCPIP    = request->hasParam("syslog.logBeaconOverTCPIP", true);
        }


        Config.tnc.enableServer             = request->hasParam("tnc.enableServer", true);
        Config.tnc.enableSerial             = request->hasParam("tnc.enableSerial", true);
        Config.tnc.acceptOwn                = request->hasParam("tnc.acceptOwn", true);


        Config.mqtt.active                  = request->hasParam("mqtt.active", true);
        if (Config.mqtt.active) {
            Config.mqtt.server              = getParamStringSafe("mqtt.server", Config.mqtt.server);
            Config.mqtt.topic               = getParamStringSafe("mqtt.topic", Config.mqtt.topic);
            Config.mqtt.username            = getParamStringSafe("mqtt.username", Config.mqtt.username);
            Config.mqtt.password            = getParamStringSafe("mqtt.password", Config.mqtt.password);
            Config.mqtt.port                = getParamIntSafe("mqtt.port", Config.mqtt.port);
        }


        Config.rebootMode                   = request->hasParam("other.rebootMode", true);
        if (Config.rebootMode) {
            Config.rebootModeTime           = getParamIntSafe("other.rebootModeTime", Config.rebootModeTime);
        }

        Config.ota.username                 = getParamStringSafe("ota.username", Config.ota.username);
        Config.ota.password                 = getParamStringSafe("ota.password", Config.ota.password);

        Config.webadmin.active              = request->hasParam("webadmin.active", true);
        if (Config.webadmin.active) {
            Config.webadmin.username        = getParamStringSafe("webadmin.username", Config.webadmin.username);
            Config.webadmin.password        = getParamStringSafe("webadmin.password", Config.webadmin.password);
        }

        Config.remoteManagement.managers    = getParamStringSafe("remoteManagement.managers", Config.remoteManagement.managers);
        Config.remoteManagement.rfOnly      = request->hasParam("remoteManagement.rfOnly", true);

        Config.ntp.gmtCorrection            = getParamFloatSafe("ntp.gmtCorrection", Config.ntp.gmtCorrection);

        Config.rememberStationTime          = getParamIntSafe("other.rememberStationTime", Config.rememberStationTime);

        Config.backupDigiMode               = request->hasParam("other.backupDigiMode", true);
        
        Serial.println("Saving configuration...");
        bool saveSuccess = Config.writeFile();

        if (saveSuccess) {
            Serial.println("Configuration saved successfully");
            AsyncWebServerResponse *response = request->beginResponse(302, "text/html", "");
            response->addHeader("Location", "/?success=1");
            request->send(response);
            
            displayToggle(false);
            delay(1000); // Dar tiempo al guardado
            ESP.restart();
        } else {
            Serial.println("Error saving configuration!");
            String errorPage = "<!DOCTYPE html><html><head><title>Error</title></head><body>";
            errorPage += "<h1>Configuration Error:</h1>";
            errorPage += "<p>Couldn't save new configuration. Please try again.</p>";
            errorPage += "<a href='/'>Back</a></body></html>";
            
            AsyncWebServerResponse *response = request->beginResponse(500, "text/html", errorPage);
            request->send(response);
        }
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
        if (Config.digi.ecoMode == 0) {
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