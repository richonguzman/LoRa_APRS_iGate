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
#include <SPIFFS.h>
#include "configuration.h"
#include "board_pinout.h"
#include "display.h"


bool shouldSleepStop = true;


void Configuration::writeFile() {
    Serial.println("Saving config...");

    StaticJsonDocument<2560> data;
    File configFile = SPIFFS.open("/igate_conf.json", "w");

    if (wifiAPs[0].ssid != "") { // We don't want to save Auto AP empty SSID
        for (int i = 0; i < wifiAPs.size(); i++) {
            data["wifi"]["AP"][i]["ssid"] = wifiAPs[i].ssid;
            data["wifi"]["AP"][i]["password"] = wifiAPs[i].password;
        }
    }

    data["wifi"]["autoAP"]["password"]          = wifiAutoAP.password;
    data["wifi"]["autoAP"]["timeout"]           = wifiAutoAP.timeout;

    data["callsign"]                            = callsign;

    data["aprs_is"]["active"]                   = aprs_is.active;
    data["aprs_is"]["passcode"]                 = aprs_is.passcode;
    data["aprs_is"]["server"]                   = aprs_is.server;
    data["aprs_is"]["port"]                     = aprs_is.port;
    data["aprs_is"]["filter"]                   = aprs_is.filter;
    data["aprs_is"]["messagesToRF"]             = aprs_is.messagesToRF;
    data["aprs_is"]["objectsToRF"]              = aprs_is.objectsToRF;

    data["beacon"]["comment"]                   = beacon.comment;
    data["beacon"]["interval"]                  = beacon.interval;
    data["beacon"]["latitude"]                  = beacon.latitude;
    data["beacon"]["longitude"]                 = beacon.longitude;
    data["beacon"]["overlay"]                   = beacon.overlay;
    data["beacon"]["symbol"]                    = beacon.symbol;
    data["beacon"]["sendViaAPRSIS"]             = beacon.sendViaAPRSIS;
    data["beacon"]["sendViaRF"]                 = beacon.sendViaRF;
    data["beacon"]["path"]                      = beacon.path;

    data["beacon"]["gpsActive"]                 = beacon.gpsActive;
    data["beacon"]["gpsAmbiguity"]              = beacon.gpsAmbiguity;

    data["digi"]["mode"]                        = digi.mode;
    data["digi"]["ecoMode"]                     = digi.ecoMode;
    #if defined(HAS_A7670)
        if (digi.ecoMode == 1) data["digi"]["ecoMode"] = 2;
    #endif

    data["lora"]["rxFreq"]                      = loramodule.rxFreq;
    data["lora"]["txFreq"]                      = loramodule.txFreq;
    data["lora"]["spreadingFactor"]             = loramodule.spreadingFactor;
    data["lora"]["signalBandwidth"]             = loramodule.signalBandwidth;
    data["lora"]["codingRate4"]                 = loramodule.codingRate4;
    data["lora"]["power"]                       = loramodule.power;
    data["lora"]["txActive"]                    = loramodule.txActive;
    data["lora"]["rxActive"]                    = loramodule.rxActive;

    data["display"]["alwaysOn"]                 = display.alwaysOn;
    data["display"]["timeout"]                  = display.timeout;
    data["display"]["turn180"]                  = display.turn180;

    data["battery"]["sendInternalVoltage"]      = battery.sendInternalVoltage;
    data["battery"]["monitorInternalVoltage"]   = battery.monitorInternalVoltage;
    data["battery"]["internalSleepVoltage"]     = battery.internalSleepVoltage;

    data["battery"]["sendExternalVoltage"]      = battery.sendExternalVoltage;
    data["battery"]["externalVoltagePin"]       = battery.externalVoltagePin;
    data["battery"]["monitorExternalVoltage"]   = battery.monitorExternalVoltage;
    data["battery"]["externalSleepVoltage"]     = battery.externalSleepVoltage;
    data["battery"]["voltageDividerR1"]         = battery.voltageDividerR1;
    data["battery"]["voltageDividerR2"]         = battery.voltageDividerR2;

    data["battery"]["sendVoltageAsTelemetry"]   = battery.sendVoltageAsTelemetry;

    data["wxsensor"]["active"]                  = wxsensor.active;
    data["wxsensor"]["heightCorrection"]        = wxsensor.heightCorrection;
    data["wxsensor"]["temperatureCorrection"]   = wxsensor.temperatureCorrection;

    data["syslog"]["active"]                    = syslog.active;
    data["syslog"]["server"]                    = syslog.server;
    data["syslog"]["port"]                      = syslog.port;

    data["tnc"]["enableServer"]                 = tnc.enableServer;
    data["tnc"]["enableSerial"]                 = tnc.enableSerial;
    data["tnc"]["acceptOwn"]                    = tnc.acceptOwn;

    data["other"]["rebootMode"]                 = rebootMode;
    data["other"]["rebootModeTime"]             = rebootModeTime;

    data["ota"]["username"]                     = ota.username;
    data["ota"]["password"]                     = ota.password;

    data["other"]["rememberStationTime"]        = rememberStationTime;

    data["other"]["backupDigiMode"]             = backupDigiMode;

    data["personalNote"]                        = personalNote;

    data["blacklist"]                           = blacklist;

    data["webadmin"]["active"]                  = webadmin.active;
    data["webadmin"]["username"]                = webadmin.username;
    data["webadmin"]["password"]                = webadmin.password;

    data["ntp"]["gmtCorrection"]                = ntp.gmtCorrection;

    data["remoteManagement"]["managers"]        = remoteManagement.managers;
    data["remoteManagement"]["rfOnly"]          = remoteManagement.rfOnly;

    serializeJson(data, configFile);

    configFile.close();

    Serial.println("Config saved");
    delay(200);
}

bool Configuration::readFile() {
    Serial.println("Reading config..");

    File configFile = SPIFFS.open("/igate_conf.json", "r");

    if (configFile) {
        StaticJsonDocument<2560> data;

        DeserializationError error = deserializeJson(data, configFile);
        if (error) {
            Serial.println("Failed to read file, using default configuration");
        }

        JsonArray WiFiArray = data["wifi"]["AP"];
        for (int i = 0; i < WiFiArray.size(); i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = WiFiArray[i]["ssid"].as<String>();
            wifiap.password               = WiFiArray[i]["password"].as<String>();

            wifiAPs.push_back(wifiap);
        }

        wifiAutoAP.password             = data["wifi"]["autoAP"]["password"] | "1234567890";
        wifiAutoAP.timeout              = data["wifi"]["autoAP"]["timeout"] | 10;

        callsign                        = data["callsign"] | "NOCALL-10";
        rememberStationTime             = data["other"]["rememberStationTime"] | 30;

        beacon.latitude                 = data["beacon"]["latitude"] | 0.0;
        beacon.longitude                = data["beacon"]["longitude"] | 0.0;
        beacon.comment                  = data["beacon"]["comment"] | "LoRa APRS";
        beacon.interval                 = data["beacon"]["interval"] | 15;
        beacon.overlay                  = data["beacon"]["overlay"] | "L";
        beacon.symbol                   = data["beacon"]["symbol"] | "a";
        beacon.path                     = data["beacon"]["path"] | "WIDE1-1";
        beacon.sendViaAPRSIS            = data["beacon"]["sendViaAPRSIS"] | false;
        beacon.sendViaRF                = data["beacon"]["sendViaRF"] | false;

        beacon.gpsActive                = data["beacon"]["gpsActive"] | false;
        beacon.gpsAmbiguity             = data["beacon"]["gpsAmbiguity"] | false;

        aprs_is.active                  = data["aprs_is"]["active"] | false;
        aprs_is.passcode                = data["aprs_is"]["passcode"] | "XYZWV";
        aprs_is.server                  = data["aprs_is"]["server"] | "rotate.aprs2.net";
        aprs_is.port                    = data["aprs_is"]["port"] | 14580;
        aprs_is.filter                  = data["aprs_is"]["filter"] | "m/10";
        aprs_is.messagesToRF            = data["aprs_is"]["messagesToRF"] | false;
        aprs_is.objectsToRF             = data["aprs_is"]["objectsToRF"] | false;

        digi.mode                       = data["digi"]["mode"] | 0;
        digi.ecoMode                    = data["digi"]["ecoMode"] | 0;
        if (digi.ecoMode == 1) shouldSleepStop = false;

        #if defined(HAS_A7670)
            if (digi.ecoMode == 1) digi.ecoMode = 2;
        #endif

        loramodule.txFreq               = data["lora"]["txFreq"] | 433775000;
        loramodule.rxFreq               = data["lora"]["rxFreq"] | 433775000;
        loramodule.spreadingFactor      = data["lora"]["spreadingFactor"] | 12;
        loramodule.signalBandwidth      = data["lora"]["signalBandwidth"] | 125000;
        loramodule.codingRate4          = data["lora"]["codingRate4"] | 5;
        loramodule.power                = data["lora"]["power"] | 20;
        loramodule.txActive             = data["lora"]["txActive"] | false;
        loramodule.rxActive             = data["lora"]["rxActive"] | false;

        display.alwaysOn                = data["display"]["alwaysOn"] | true;
        display.timeout                 = data["display"]["timeout"] | 4;
        display.turn180                 = data["display"]["turn180"] | false;

        battery.sendInternalVoltage     = data["battery"]["sendInternalVoltage"] | false;
        battery.monitorInternalVoltage  = data["battery"]["monitorInternalVoltage"] | false;
        battery.internalSleepVoltage    = data["battery"]["internalSleepVoltage"] | 2.9;

        battery.sendExternalVoltage     = data["battery"]["sendExternalVoltage"] | false;
        battery.externalVoltagePin      = data["battery"]["externalVoltagePin"] | 34;
        battery.monitorExternalVoltage  = data["battery"]["monitorExternalVoltage"] | false;
        battery.externalSleepVoltage    = data["battery"]["externalSleepVoltage"] | 10.9;
        battery.voltageDividerR1        = data["battery"]["voltageDividerR1"] | 100.0;
        battery.voltageDividerR2        = data["battery"]["voltageDividerR2"] | 27.0;

        battery.sendVoltageAsTelemetry  = data["battery"]["sendVoltageAsTelemetry"] | false;

        wxsensor.active                 = data["wxsensor"]["active"] | false;
        wxsensor.heightCorrection       = data["wxsensor"]["heightCorrection"] | 0;
        wxsensor.temperatureCorrection  = data["wxsensor"]["temperatureCorrection"] | 0.0;

        syslog.active                   = data["syslog"]["active"] | false;
        syslog.server                   = data["syslog"]["server"] | "lora.link9.net";
        syslog.port                     = data["syslog"]["port"] | 1514;

        tnc.enableServer                = data["tnc"]["enableServer"] | false;
        tnc.enableSerial                = data["tnc"]["enableSerial"] | false;
        tnc.acceptOwn                   = data["tnc"]["acceptOwn"] | false;

        ota.username                    = data["ota"]["username"] | "";
        ota.password                    = data["ota"]["password"] | "";

        webadmin.active                 = data["webadmin"]["active"] | false;
        webadmin.username               = data["webadmin"]["username"] | "admin";
        webadmin.password               = data["webadmin"]["password"] | "";

        ntp.gmtCorrection               = data["ntp"]["gmtCorrection"] | 0.0;

        backupDigiMode                  = data["other"]["backupDigiMode"] | false;

        rebootMode                      = data["other"]["rebootMode"] | false;
        rebootModeTime                  = data["other"]["rebootModeTime"] | 6;

        personalNote    	            = data["personalNote"] | "personal note here";

        blacklist                       = data["blacklist"] | "station callsign";

        remoteManagement.managers       = data["remoteManagement"]["managers"] | "";
        remoteManagement.rfOnly         = data["remoteManagement"]["rfOnly"] | true;

        if (wifiAPs.size() == 0) { // If we don't have any WiFi's from config we need to add "empty" SSID for AUTO AP
            WiFi_AP wifiap;
            wifiap.ssid = "";
            wifiap.password = "";

            wifiAPs.push_back(wifiap);
        }
        configFile.close();
        Serial.println("Config read successfuly");
        return true;
    } else {
        Serial.println("Config file not found");
        return false;
    }
}
    
void Configuration::init() {

    WiFi_AP wifiap;
    wifiap.ssid                     = "";
    wifiap.password                 = "";

    wifiAPs.push_back(wifiap);

    wifiAutoAP.password             = "1234567890";
    wifiAutoAP.timeout              = 10;

    callsign                        = "N0CALL-10";

    beacon.comment                  = "LoRa APRS";
    beacon.latitude                 = 0.0;
    beacon.longitude                = 0.0;
    beacon.interval                 = 15;
    beacon.overlay                  = "L";
    beacon.symbol                   = "a";
    beacon.sendViaAPRSIS            = true;
    beacon.sendViaRF                = false;
    beacon.path                     = "WIDE1-1";

    beacon.gpsActive                = false;
    beacon.gpsAmbiguity             = false;

    digi.mode                       = 0;
    digi.ecoMode                    = 0;

    tnc.enableServer                = false;
    tnc.enableSerial                = false;
    tnc.acceptOwn                   = false;

    aprs_is.active                  = false;
    aprs_is.passcode                = "XYZVW";
    aprs_is.server                  = "rotate.aprs2.net";
    aprs_is.port                    = 14580;
    aprs_is.filter                  = "m/10";
    aprs_is.messagesToRF            = false;
    aprs_is.objectsToRF             = false;

    loramodule.txFreq               = 433775000;
    loramodule.rxFreq               = 433775000;
    loramodule.spreadingFactor      = 12;
    loramodule.signalBandwidth      = 125000;
    loramodule.codingRate4          = 5;
    loramodule.power                = 20;
    loramodule.txActive             = false;
    loramodule.rxActive             = true;

    display.alwaysOn                = true;
    display.timeout                 = 4;
    display.turn180                 = false;

    syslog.active                   = false;
    syslog.server                   = "lora.link9.net";
    syslog.port                     = 1514;

    wxsensor.active                 = false;
    wxsensor.heightCorrection       = 0;
    wxsensor.temperatureCorrection  = 0.0;

    ota.username                    = "";
    ota.password                    = "";

    rememberStationTime             = 30;

    battery.sendInternalVoltage     = false;
    battery.monitorInternalVoltage  = false;
    battery.internalSleepVoltage    = 2.9;

    battery.sendExternalVoltage     = false;
    battery.externalVoltagePin      = 34;
    battery.monitorExternalVoltage  = false;
    battery.externalSleepVoltage    = 10.9;
    battery.voltageDividerR1        = 100.0;
    battery.voltageDividerR2        = 27.0;

    battery.sendVoltageAsTelemetry  = false;

    backupDigiMode                  = false;

    rebootMode                      = false;
    rebootModeTime                  = 0;

    personalNote                    = "";

    blacklist                       = "";

    webadmin.active                 = false;
    webadmin.username               = "admin";
    webadmin.password               = "";

    ntp.gmtCorrection               = 0.0;

    remoteManagement.managers       = "";
    remoteManagement.rfOnly         = true;

    Serial.println("All is Written!");
}

Configuration::Configuration() {
    if (!SPIFFS.begin(false)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("SPIFFS Mounted");
    }

    bool exists = SPIFFS.exists("/igate_conf.json");
    if (!exists) {
        init();
        writeFile();
        ESP.restart();
    }

    readFile();
}