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


bool Configuration::writeFile() {
    Serial.println("Saving configuration...");

    JsonDocument data;
    File configFile = SPIFFS.open("/igate_conf.json", "w");

    if (!configFile) {
        Serial.println("Error: Could not open config file for writing");
        return false;
    }
    try {

        if (wifiAPs[0].ssid != "") { // We don't want to save Auto AP empty SSID
            for (int i = 0; i < wifiAPs.size(); i++) {
                data["wifi"]["AP"][i]["ssid"] = wifiAPs[i].ssid;
                data["wifi"]["AP"][i]["password"] = wifiAPs[i].password;
            }
        }

        data["other"]["startupDelay"]               = startupDelay;

        data["wifi"]["autoAP"]["enabled"]           = wifiAutoAP.enabled;
        data["wifi"]["autoAP"]["password"]          = wifiAutoAP.password;
        data["wifi"]["autoAP"]["timeout"]           = wifiAutoAP.timeout;

        callsign.trim();
        data["callsign"]                            = callsign;
        tacticalCallsign.trim();
        data["tacticalCallsign"]                    = tacticalCallsign;

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
        data["beacon"]["path"]                      = beacon.path;

        data["beacon"]["sendViaAPRSIS"]             = beacon.sendViaAPRSIS;
        data["beacon"]["sendViaRF"]                 = beacon.sendViaRF;
        data["beacon"]["beaconFreq"]                = beacon.beaconFreq;

        data["beacon"]["statusActive"]              = beacon.statusActive;
        data["beacon"]["statusPacket"]              = beacon.statusPacket;

        data["beacon"]["gpsActive"]                 = beacon.gpsActive;
        data["beacon"]["ambiguityLevel"]            = beacon.ambiguityLevel;

        data["personalNote"]                        = personalNote;

        data["blacklist"]                           = blacklist;

        data["digi"]["mode"]                        = digi.mode;
        data["digi"]["ecoMode"]                     = digi.ecoMode;
        if (digi.ecoMode == 1) data["aprs_is"]["active"] = false;
        #if defined(HAS_A7670)
            if (digi.ecoMode == 1) data["digi"]["ecoMode"] = 2;
        #endif
        data["digi"]["backupDigiMode"]              = digi.backupDigiMode;

        data["lora"]["rxActive"]                    = loramodule.rxActive;
        data["lora"]["rxFreq"]                      = loramodule.rxFreq;
        data["lora"]["rxCodingRate4"]               = loramodule.rxCodingRate4;
        data["lora"]["rxSignalBandwidth"]           = loramodule.rxSignalBandwidth;
        data["lora"]["txActive"]                    = loramodule.txActive;
        data["lora"]["txFreq"]                      = loramodule.txFreq;
        data["lora"]["txCodingRate4"]               = loramodule.txCodingRate4;
        data["lora"]["txSignalBandwidth"]           = loramodule.txSignalBandwidth;
        data["lora"]["power"]                       = loramodule.power;

        int rxSpreadingFactor = loramodule.rxSpreadingFactor;
        int txSpreadingFactor = loramodule.txSpreadingFactor;
        #if defined(HAS_SX1276) || defined(HAS_SX1278)
            const int minSF = 6, maxSF = 12;
        #endif
        #if defined(HAS_SX1262) || defined(HAS_SX1268)
            const int minSF = 5, maxSF = 12;
        #endif
        #if defined(HAS_LLCC68)
            const int minSF = 5, maxSF = 11;
        #endif

        rxSpreadingFactor = (rxSpreadingFactor < minSF) ? minSF : (rxSpreadingFactor > maxSF) ? maxSF : rxSpreadingFactor;
        txSpreadingFactor = (txSpreadingFactor < minSF) ? minSF : (txSpreadingFactor > maxSF) ? maxSF : txSpreadingFactor;

        data["lora"]["rxSpreadingFactor"]           = rxSpreadingFactor;
        data["lora"]["txSpreadingFactor"]           = txSpreadingFactor;

        data["display"]["alwaysOn"]                 = display.alwaysOn;
        data["display"]["timeout"]                  = display.timeout;
        data["display"]["turn180"]                  = display.turn180;

        data["battery"]["sendInternalVoltage"]      = battery.sendInternalVoltage;
        data["battery"]["monitorInternalVoltage"]   = battery.monitorInternalVoltage;
        data["battery"]["internalSleepVoltage"]     = battery.internalSleepVoltage;

        data["battery"]["sendExternalVoltage"]      = battery.sendExternalVoltage;
        data["battery"]["monitorExternalVoltage"]   = battery.monitorExternalVoltage;
        data["battery"]["externalSleepVoltage"]     = battery.externalSleepVoltage;
        data["battery"]["useExternalI2CSensor"]     = battery.useExternalI2CSensor;
        data["battery"]["voltageDividerR1"]         = battery.voltageDividerR1;
        data["battery"]["voltageDividerR2"]         = battery.voltageDividerR2;
        data["battery"]["externalVoltagePin"]       = battery.externalVoltagePin;

        data["battery"]["sendVoltageAsTelemetry"]   = battery.sendVoltageAsTelemetry;

        data["wxsensor"]["active"]                  = wxsensor.active;
        data["wxsensor"]["heightCorrection"]        = wxsensor.heightCorrection;
        data["wxsensor"]["temperatureCorrection"]   = wxsensor.temperatureCorrection;

        data["syslog"]["active"]                    = syslog.active;
        data["syslog"]["server"]                    = syslog.server;
        data["syslog"]["port"]                      = syslog.port;
        data["syslog"]["logBeaconOverTCPIP"]        = syslog.logBeaconOverTCPIP;

        data["tnc"]["enableServer"]                 = tnc.enableServer;
        data["tnc"]["enableSerial"]                 = tnc.enableSerial;
        data["tnc"]["acceptOwn"]                    = tnc.acceptOwn;
        data["tnc"]["aprsBridgeActive"]             = tnc.aprsBridgeActive;

        data["mqtt"]["active"]                      = mqtt.active;
        data["mqtt"]["server"]                      = mqtt.server;
        data["mqtt"]["topic"]                       = mqtt.topic;
        data["mqtt"]["username"]                    = mqtt.username;
        data["mqtt"]["password"]                    = mqtt.password;
        data["mqtt"]["port"]                        = mqtt.port;
        data["mqtt"]["beaconOverMqtt"]              = mqtt.beaconOverMqtt;

        data["ota"]["username"]                     = ota.username;
        data["ota"]["password"]                     = ota.password;

        data["webadmin"]["active"]                  = webadmin.active;
        data["webadmin"]["username"]                = webadmin.username;
        data["webadmin"]["password"]                = webadmin.password;

        data["remoteManagement"]["managers"]        = remoteManagement.managers;
        data["remoteManagement"]["rfOnly"]          = remoteManagement.rfOnly;

        data["ntp"]["server"]                       = ntp.server;
        data["ntp"]["gmtCorrection"]                = ntp.gmtCorrection;

        data["other"]["rebootMode"]                 = rebootMode;
        data["other"]["rebootModeTime"]             = rebootModeTime;

        data["other"]["rememberStationTime"]        = rememberStationTime;

        serializeJson(data, configFile);
        configFile.close();
        return true;
    } catch (...) {
        Serial.println("Error: Exception occurred while saving config");
        configFile.close();
        return false;
    }
}

bool Configuration::readFile() {
    Serial.println("Reading config..");
    File configFile = SPIFFS.open("/igate_conf.json", "r");

    if (configFile) {
        bool needsRewrite = false;
        JsonDocument data;
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

        if (data["other"]["startupDelay"].isNull()) needsRewrite = true;
        startupDelay                    = data["other"]["startupDelay"] | 0;

        if (data["wifi"]["autoAP"]["enabled"].isNull() ||
            data["wifi"]["autoAP"]["password"].isNull() ||
            data["wifi"]["autoAP"]["timeout"].isNull()) needsRewrite = true;
        wifiAutoAP.enabled              = data["wifi"]["autoAP"]["enabled"] | true;
        wifiAutoAP.password             = data["wifi"]["autoAP"]["password"] | "1234567890";
        wifiAutoAP.timeout              = data["wifi"]["autoAP"]["timeout"] | 10;

        if (data["callsign"].isNull()) needsRewrite = true;
        callsign                        = data["callsign"] | "NOCALL-10";
        if (data["tacticalCallsign"].isNull()) needsRewrite = true;
        tacticalCallsign                = data["tacticalCallsign"] | "";

        if (data["aprs_is"]["active"].isNull() ||
            data["aprs_is"]["passcode"].isNull() ||
            data["aprs_is"]["server"].isNull() ||
            data["aprs_is"]["port"].isNull() ||
            data["aprs_is"]["filter"].isNull() ||
            data["aprs_is"]["messagesToRF"].isNull() ||
            data["aprs_is"]["objectsToRF"].isNull()) needsRewrite = true;
        aprs_is.active                  = data["aprs_is"]["active"] | false;
        aprs_is.passcode                = data["aprs_is"]["passcode"] | "XYZWV";
        aprs_is.server                  = data["aprs_is"]["server"] | "rotate.aprs2.net";
        aprs_is.port                    = data["aprs_is"]["port"] | 14580;
        aprs_is.filter                  = data["aprs_is"]["filter"] | "m/10";
        aprs_is.messagesToRF            = data["aprs_is"]["messagesToRF"] | false;
        aprs_is.objectsToRF             = data["aprs_is"]["objectsToRF"] | false;

        if (data["beacon"]["latitude"].isNull() ||
            data["beacon"]["longitude"].isNull() ||
            data["beacon"]["comment"].isNull() ||
            data["beacon"]["interval"].isNull() ||
            data["beacon"]["overlay"].isNull() ||
            data["beacon"]["symbol"].isNull() ||
            data["beacon"]["path"].isNull() ||
            data["beacon"]["sendViaAPRSIS"].isNull() ||
            data["beacon"]["sendViaRF"].isNull() ||
            data["beacon"]["beaconFreq"].isNull() ||
            data["beacon"]["statusActive"].isNull() ||
            data["beacon"]["statusPacket"].isNull() ||
            data["beacon"]["gpsActive"].isNull() ||
            data["beacon"]["ambiguityLevel"].isNull()) needsRewrite = true;
        beacon.latitude                 = data["beacon"]["latitude"] | 0.0;
        beacon.longitude                = data["beacon"]["longitude"] | 0.0;
        beacon.comment                  = data["beacon"]["comment"] | "LoRa APRS";
        beacon.interval                 = data["beacon"]["interval"] | 15;
        beacon.overlay                  = data["beacon"]["overlay"] | "L";
        beacon.symbol                   = data["beacon"]["symbol"] | "a";
        beacon.path                     = data["beacon"]["path"] | "WIDE1-1";
        beacon.sendViaAPRSIS            = data["beacon"]["sendViaAPRSIS"] | false;
        beacon.sendViaRF                = data["beacon"]["sendViaRF"] | false;
        beacon.beaconFreq               = data["beacon"]["beaconFreq"] | 1;
        beacon.statusActive             = data["beacon"]["statusActive"] | false;
        beacon.statusPacket             = data["beacon"]["statusPacket"] | "";
        beacon.gpsActive                = data["beacon"]["gpsActive"] | false;
        beacon.ambiguityLevel           = data["beacon"]["ambiguityLevel"] | 0;

        if (data["personalNote"].isNull()) needsRewrite = true;
        personalNote    	            = data["personalNote"] | "personal note here";

        if (data["blacklist"].isNull()) needsRewrite = true;
        blacklist                       = data["blacklist"] | "station callsign";

        if (data["digi"]["mode"].isNull() ||
            data["digi"]["ecoMode"].isNull() ||
            data["digi"]["backupDigiMode"].isNull()) needsRewrite = true;
        digi.mode                       = data["digi"]["mode"] | 0;
        digi.ecoMode                    = data["digi"]["ecoMode"] | 0;
        if (digi.ecoMode == 1) shouldSleepStop = false;
        #if defined(HAS_A7670)
            if (digi.ecoMode == 1) digi.ecoMode = 2;
        #endif
        digi.backupDigiMode             = data["digi"]["backupDigiMode"] | false;


        if (data["lora"]["rxActive"].isNull() ||
            data["lora"]["rxFreq"].isNull() ||
            data["lora"]["rxSpreadingFactor"].isNull() ||
            data["lora"]["rxCodingRate4"].isNull() ||
            data["lora"]["rxSignalBandwidth"].isNull() ||
            data["lora"]["txActive"].isNull() ||
            data["lora"]["txFreq"].isNull() ||
            data["lora"]["txSpreadingFactor"].isNull() ||
            data["lora"]["txCodingRate4"].isNull() ||
            data["lora"]["txSignalBandwidth"].isNull() ||
            data["lora"]["power"].isNull()) needsRewrite = true;
        loramodule.rxActive             = data["lora"]["rxActive"] | true;
        loramodule.rxFreq               = data["lora"]["rxFreq"] | 433775000;
        loramodule.rxSpreadingFactor    = data["lora"]["rxSpreadingFactor"] | 12;
        loramodule.rxCodingRate4        = data["lora"]["rxCodingRate4"] | 5;
        loramodule.rxSignalBandwidth    = data["lora"]["rxSignalBandwidth"] | 125000;
        loramodule.txActive             = data["lora"]["txActive"] | false;
        loramodule.txFreq               = data["lora"]["txFreq"] | 433775000;
        loramodule.txSpreadingFactor    = data["lora"]["txSpreadingFactor"] | 12;
        loramodule.txCodingRate4        = data["lora"]["txCodingRate4"] | 5;
        loramodule.txSignalBandwidth    = data["lora"]["txSignalBandwidth"] | 125000;
        loramodule.power                = data["lora"]["power"] | 20;

        if (data["display"]["alwaysOn"].isNull() ||
            data["display"]["timeout"].isNull() ||
            data["display"]["turn180"].isNull()) needsRewrite = true;
        #ifdef HAS_EPAPER
            display.alwaysOn            = true;
        #else
            display.alwaysOn            = data["display"]["alwaysOn"] | true;
        #endif
        display.timeout                 = data["display"]["timeout"] | 4;
        display.turn180                 = data["display"]["turn180"] | false;

        if (data["battery"]["sendInternalVoltage"].isNull() ||
            data["battery"]["monitorInternalVoltage"].isNull() ||
            data["battery"]["internalSleepVoltage"].isNull() ||
            data["battery"]["sendExternalVoltage"].isNull() ||
            data["battery"]["monitorExternalVoltage"].isNull() ||
            data["battery"]["externalSleepVoltage"].isNull() ||
            data["battery"]["useExternalI2CSensor"].isNull() ||
            data["battery"]["voltageDividerR1"].isNull() ||
            data["battery"]["voltageDividerR2"].isNull() ||
            data["battery"]["externalVoltagePin"].isNull() ||
            data["battery"]["sendVoltageAsTelemetry"].isNull()) needsRewrite = true;
        battery.sendInternalVoltage     = data["battery"]["sendInternalVoltage"] | false;
        battery.monitorInternalVoltage  = data["battery"]["monitorInternalVoltage"] | false;
        battery.internalSleepVoltage    = data["battery"]["internalSleepVoltage"] | 2.9;
        battery.sendExternalVoltage     = data["battery"]["sendExternalVoltage"] | false;
        battery.monitorExternalVoltage  = data["battery"]["monitorExternalVoltage"] | false;
        battery.externalSleepVoltage    = data["battery"]["externalSleepVoltage"] | 10.9;
        battery.useExternalI2CSensor    = data["battery"]["useExternalI2CSensor"] | false;
        battery.voltageDividerR1        = data["battery"]["voltageDividerR1"] | 100.0;
        battery.voltageDividerR2        = data["battery"]["voltageDividerR2"] | 27.0;
        battery.externalVoltagePin      = data["battery"]["externalVoltagePin"] | 34;
        battery.sendVoltageAsTelemetry  = data["battery"]["sendVoltageAsTelemetry"] | false;

        if (data["wxsensor"]["active"].isNull() ||
            data["wxsensor"]["heightCorrection"].isNull() ||
            data["wxsensor"]["temperatureCorrection"].isNull()) needsRewrite = true;
        wxsensor.active                 = data["wxsensor"]["active"] | false;
        wxsensor.heightCorrection       = data["wxsensor"]["heightCorrection"] | 0;
        wxsensor.temperatureCorrection  = data["wxsensor"]["temperatureCorrection"] | 0.0;

        if (data["syslog"]["active"].isNull() ||
            data["syslog"]["server"].isNull() ||
            data["syslog"]["port"].isNull() ||
            data["syslog"]["logBeaconOverTCPIP"].isNull()) needsRewrite = true;
        syslog.active                   = data["syslog"]["active"] | false;
        syslog.server                   = data["syslog"]["server"] | "lora.link9.net";
        syslog.port                     = data["syslog"]["port"] | 1514;
        syslog.logBeaconOverTCPIP       = data["syslog"]["logBeaconOverTCPIP"] | false;

        if (data["tnc"]["enableServer"].isNull() ||
            data["tnc"]["enableSerial"].isNull() ||
            data["tnc"]["acceptOwn"].isNull() ||
            data["tnc"]["aprsBridgeActive"].isNull()) needsRewrite = true;
        tnc.enableServer                = data["tnc"]["enableServer"] | false;
        tnc.enableSerial                = data["tnc"]["enableSerial"] | false;
        tnc.acceptOwn                   = data["tnc"]["acceptOwn"] | false;
        tnc.aprsBridgeActive            = data["tnc"]["aprsBridgeActive"] | false;

        if (data["mqtt"]["active"].isNull() ||
            data["mqtt"]["server"].isNull() ||
            data["mqtt"]["topic"].isNull() ||
            data["mqtt"]["username"].isNull() ||
            data["mqtt"]["password"].isNull() ||
            data["mqtt"]["port"].isNull() ||
            data["mqtt"]["beaconOverMqtt"].isNull()) needsRewrite = true;
        mqtt.active                     = data["mqtt"]["active"] | false;
        mqtt.server                     = data["mqtt"]["server"] | "";
        mqtt.topic                      = data["mqtt"]["topic"] | "aprs-igate";
        mqtt.username                   = data["mqtt"]["username"] | "";
        mqtt.password                   = data["mqtt"]["password"] | "";
        mqtt.port                       = data["mqtt"]["port"] | 1883;
        mqtt.beaconOverMqtt             = data["mqtt"]["beaconOverMqtt"] | false;

        if (data["ota"]["username"].isNull() ||
            data["ota"]["password"].isNull()) needsRewrite = true;
        ota.username                    = data["ota"]["username"] | "";
        ota.password                    = data["ota"]["password"] | "";

        if (data["webadmin"]["active"].isNull() ||
            data["webadmin"]["username"].isNull() ||
            data["webadmin"]["password"].isNull()) needsRewrite = true;
        webadmin.active                 = data["webadmin"]["active"] | false;
        webadmin.username               = data["webadmin"]["username"] | "admin";
        webadmin.password               = data["webadmin"]["password"] | "";

        if (data["remoteManagement"]["managers"].isNull() ||
            data["remoteManagement"]["rfOnly"].isNull()) needsRewrite = true;
        remoteManagement.managers       = data["remoteManagement"]["managers"] | "";
        remoteManagement.rfOnly         = data["remoteManagement"]["rfOnly"] | true;

        if (data["ntp"]["server"].isNull() ||
            data["ntp"]["gmtCorrection"].isNull()) needsRewrite = true;
        ntp.server                      = data["ntp"]["server"] | "pool.ntp.org";
        ntp.gmtCorrection               = data["ntp"]["gmtCorrection"] | 0.0;

        if (data["other"]["rebootMode"].isNull() ||
            data["other"]["rebootModeTime"].isNull()) needsRewrite = true;
        rebootMode                      = data["other"]["rebootMode"] | false;
        rebootModeTime                  = data["other"]["rebootModeTime"] | 6;

        if (data["other"]["rememberStationTime"].isNull()) needsRewrite = true;
        rememberStationTime             = data["other"]["rememberStationTime"] | 30;

        if (wifiAPs.size() == 0) { // If we don't have any WiFi's from config we need to add "empty" SSID for AUTO AP
            WiFi_AP wifiap;
            wifiap.ssid = "";
            wifiap.password = "";

            wifiAPs.push_back(wifiap);
        }
        configFile.close();

        if (needsRewrite) {
            Serial.println("Config JSON incomplete, rewriting...");
            writeFile();
            delay(1000);
            ESP.restart();
        }
        Serial.println("Config read successfuly");
        return true;
    } else {
        Serial.println("Config file not found");
        return false;
    }
}

void Configuration::setDefaultValues() {

    WiFi_AP wifiap;
    wifiap.ssid                     = "";
    wifiap.password                 = "";

    wifiAPs.push_back(wifiap);

    startupDelay                    = 0;

    wifiAutoAP.enabled              = true;
    wifiAutoAP.password             = "1234567890";
    wifiAutoAP.timeout              = 10;

    callsign                        = "N0CALL-10";
    tacticalCallsign                = "";

    aprs_is.active                  = false;
    aprs_is.passcode                = "XYZVW";
    aprs_is.server                  = "rotate.aprs2.net";
    aprs_is.port                    = 14580;
    aprs_is.filter                  = "m/10";
    aprs_is.messagesToRF            = false;
    aprs_is.objectsToRF             = false;

    beacon.comment                  = "LoRa APRS";
    beacon.latitude                 = 0.0;
    beacon.longitude                = 0.0;
    beacon.interval                 = 15;
    beacon.overlay                  = "L";
    beacon.symbol                   = "a";
    beacon.path                     = "WIDE1-1";

    beacon.sendViaAPRSIS            = true;
    beacon.sendViaRF                = false;
    beacon.beaconFreq               = 1;

    beacon.statusActive             = false;
    beacon.statusPacket             = "";

    beacon.gpsActive                = false;
    beacon.ambiguityLevel           = 0;

    personalNote                    = "";

    blacklist                       = "";

    digi.mode                       = 0;
    digi.ecoMode                    = 0;
    digi.backupDigiMode             = false;

    loramodule.rxActive             = true;
    loramodule.rxFreq               = 433775000;
    loramodule.rxSpreadingFactor    = 12;
    loramodule.rxCodingRate4        = 5;
    loramodule.rxSignalBandwidth    = 125000;
    loramodule.txActive             = false;
    loramodule.txFreq               = 433775000;
    loramodule.txSpreadingFactor    = 12;
    loramodule.txCodingRate4        = 5;
    loramodule.txSignalBandwidth    = 125000;
    loramodule.power                = 20;

    display.alwaysOn                = true;
    display.timeout                 = 4;
    display.turn180                 = false;

    battery.sendInternalVoltage     = false;
    battery.monitorInternalVoltage  = false;
    battery.internalSleepVoltage    = 2.9;

    battery.sendExternalVoltage     = false;
    battery.monitorExternalVoltage  = false;
    battery.externalSleepVoltage    = 10.9;
    battery.useExternalI2CSensor    = false;
    battery.voltageDividerR1        = 100.0;
    battery.voltageDividerR2        = 27.0;
    battery.externalVoltagePin      = 34;

    battery.sendVoltageAsTelemetry  = false;

    wxsensor.active                 = false;
    wxsensor.heightCorrection       = 0;
    wxsensor.temperatureCorrection  = 0.0;

    syslog.active                   = false;
    syslog.server                   = "lora.link9.net";
    syslog.port                     = 1514;
    syslog.logBeaconOverTCPIP       = false;

    tnc.enableServer                = false;
    tnc.enableSerial                = false;
    tnc.acceptOwn                   = false;
    tnc.aprsBridgeActive            = false;

    mqtt.active                     = false;
    mqtt.server                     = "";
    mqtt.topic                      = "aprs-igate";
    mqtt.username                   = "";
    mqtt.password                   = "";
    mqtt.port                       = 1883;
    mqtt.beaconOverMqtt             = false;

    ota.username                    = "";
    ota.password                    = "";

    webadmin.active                 = false;
    webadmin.username               = "admin";
    webadmin.password               = "";

    remoteManagement.managers       = "";
    remoteManagement.rfOnly         = true;

    ntp.server                      = "pool.ntp.org";
    ntp.gmtCorrection               = 0.0;

    rebootMode                      = false;
    rebootModeTime                  = 0;

    rememberStationTime             = 30;

    Serial.println("New Data Created... All is Written!");
}

void Configuration::setup() {
    if (!SPIFFS.begin(false)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("SPIFFS Mounted");
    }

    bool exists = SPIFFS.exists("/igate_conf.json");
    if (!exists) {
        setDefaultValues();
        writeFile();
        delay(1000);
        ESP.restart();
    }

    readFile();
}