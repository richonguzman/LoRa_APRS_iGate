#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "configuration.h"
#include "display.h"


void Configuration::writeFile() {
    Serial.println("Saving config..");

    StaticJsonDocument<1536> data;
    File configFile = SPIFFS.open("/igate_conf.json", "w");

    if (wifiAPs[0].ssid != "") { // We don't want to save Auto AP empty SSID
        for (int i = 0; i < wifiAPs.size(); i++) {
            data["wifi"]["AP"][i]["ssid"] = wifiAPs[i].ssid;
            data["wifi"]["AP"][i]["password"] = wifiAPs[i].password;
            data["wifi"]["AP"][i]["latitude"] = wifiAPs[i].latitude;
            data["wifi"]["AP"][i]["longitude"] = wifiAPs[i].longitude;
        }
    }

    data["wifi"]["autoAP"]["password"] = wifiAutoAP.password;
    data["wifi"]["autoAP"]["powerOff"] = wifiAutoAP.powerOff;

    data["callsign"] = callsign;
    data["stationMode"] = stationMode;
    data["iGateComment"] = iGateComment;

    data["other"]["beaconInterval"] = beaconInterval;
    data["other"]["igateSendsLoRaBeacons"] = igateSendsLoRaBeacons;
    data["other"]["igateRepeatsLoRaPackets"] = igateRepeatsLoRaPackets;
    data["other"]["rememberStationTime"] = rememberStationTime;
    data["other"]["sendBatteryVoltage"] = sendBatteryVoltage;
    data["other"]["externalVoltageMeasurement"] = externalVoltageMeasurement;
    data["other"]["externalVoltagePin"] = externalVoltagePin;

    data["digi"]["comment"] = digi.comment;
    data["digi"]["latitude"] = digi.latitude;
    data["digi"]["longitude"] = digi.longitude;

    data["aprs_is"]["passcode"] = aprs_is.passcode;
    data["aprs_is"]["server"] = aprs_is.server;
    data["aprs_is"]["port"] = aprs_is.port;
    data["aprs_is"]["reportingDistance"] = aprs_is.reportingDistance;

    data["lora"]["iGateFreq"] = loramodule.iGateFreq;
    data["lora"]["digirepeaterTxFreq"] = loramodule.digirepeaterTxFreq;
    data["lora"]["digirepeaterRxFreq"] = loramodule.digirepeaterRxFreq;
    data["lora"]["spreadingFactor"] = loramodule.spreadingFactor;
    data["lora"]["signalBandwidth"] = loramodule.signalBandwidth;
    data["lora"]["codingRate4"] = loramodule.codingRate4;
    data["lora"]["power"] = loramodule.power;

    data["display"]["alwaysOn"] = display.alwaysOn;
    data["display"]["timeout"] = display.timeout;
    data["display"]["turn180"] = display.turn180;

    data["syslog"]["active"] = syslog.active;
    data["syslog"]["server"] = syslog.server;
    data["syslog"]["port"] = syslog.port;

    data["bme"]["active"] = bme.active;

    data["ota"]["username"] = ota.username;
    data["ota"]["password"] = ota.password;
    
    serializeJson(data, configFile);

    configFile.close();

    Serial.println("Config saved");
}

bool Configuration::readFile() {
    Serial.println("Reading config..");

    File configFile = SPIFFS.open("/igate_conf.json", "r");

    if (configFile) {
        StaticJsonDocument<1536> data;

        DeserializationError error = deserializeJson(data, configFile);
        if (error) {
            Serial.println("Failed to read file, using default configuration");
        }

        JsonArray WiFiArray = data["wifi"]["AP"];
        for (int i = 0; i < WiFiArray.size(); i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = WiFiArray[i]["ssid"].as<String>();
            wifiap.password               = WiFiArray[i]["password"].as<String>();
            wifiap.latitude               = WiFiArray[i]["latitude"].as<double>();
            wifiap.longitude              = WiFiArray[i]["longitude"].as<double>();

            wifiAPs.push_back(wifiap);
        }

        wifiAutoAP.password             = data["wifi"]["autoAP"]["password"].as<String>();
        wifiAutoAP.powerOff             = data["wifi"]["autoAP"]["powerOff"].as<int>();

        callsign                        = data["callsign"].as<String>();
        stationMode                     = data["stationMode"].as<int>();
        iGateComment                    = data["iGateComment"].as<String>();
        beaconInterval                  = data["other"]["beaconInterval"].as<int>();
        igateSendsLoRaBeacons           = data["other"]["igateSendsLoRaBeacons"].as<bool>();
        igateRepeatsLoRaPackets         = data["other"]["igateRepeatsLoRaPackets"].as<bool>();
        rememberStationTime             = data["other"]["rememberStationTime"].as<int>();
        sendBatteryVoltage              = data["other"]["sendBatteryVoltage"].as<bool>();
        externalVoltageMeasurement      = data["other"]["externalVoltageMeasurement"].as<bool>();
        externalVoltagePin              = data["other"]["externalVoltagePin"].as<int>();

        digi.comment                    = data["digi"]["comment"].as<String>();
        digi.latitude                   = data["digi"]["latitude"].as<double>();
        digi.longitude                  = data["digi"]["longitude"].as<double>();

        aprs_is.passcode                = data["aprs_is"]["passcode"].as<String>();
        aprs_is.server                  = data["aprs_is"]["server"].as<String>();
        aprs_is.port                    = data["aprs_is"]["port"].as<int>();
        aprs_is.reportingDistance       = data["aprs_is"]["reportingDistance"].as<int>();

        loramodule.iGateFreq            = data["lora"]["iGateFreq"].as<long>();
        loramodule.digirepeaterTxFreq   = data["lora"]["digirepeaterTxFreq"].as<long>();
        loramodule.digirepeaterRxFreq   = data["lora"]["digirepeaterRxFreq"].as<long>();
        loramodule.spreadingFactor      = data["lora"]["spreadingFactor"].as<int>();
        loramodule.signalBandwidth      = data["lora"]["signalBandwidth"].as<long>();
        loramodule.codingRate4          = data["lora"]["codingRate4"].as<int>();
        loramodule.power                = data["lora"]["power"].as<int>();

        display.alwaysOn                = data["display"]["alwaysOn"].as<bool>();
        display.timeout                 = data["display"]["timeout"].as<int>();
        display.turn180                 = data["display"]["turn180"].as<bool>();

        syslog.active                   = data["syslog"]["active"].as<bool>();
        syslog.server                   = data["syslog"]["server"].as<String>();
        syslog.port                     = data["syslog"]["port"].as<int>();

        bme.active                      = data["bme"]["active"].as<bool>();

        ota.username                    = data["ota"]["username"].as<String>();
        ota.password                    = data["ota"]["password"].as<String>();

        if (wifiAPs.size() == 0) { // If we don't have any WiFi's from config we need to add "empty" SSID for AUTO AP
            WiFi_AP wifiap;
            wifiap.ssid = "";
            wifiap.password = "";
            wifiap.latitude = 0.0;
            wifiap.longitude = 0.0;

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
    wifiap.ssid                   = "";
    wifiap.password               = "";
    wifiap.latitude               = 0.0;
    wifiap.longitude              = 0.0;
    wifiAPs.push_back(wifiap);

    wifiAutoAP.password = "1234567890";
    wifiAutoAP.powerOff = 15;

    callsign = "N0CALL";
    stationMode = 1;
    iGateComment = "LoRa_APRS_iGate Development";

    digi.comment = "LoRa_APRS_iGate Development";
    digi.latitude = 0.0;
    digi.longitude = 0.0;

    aprs_is.passcode = "XYZVW";
    aprs_is.server = "rotate.aprs2.net";
    aprs_is.port = 14580;
    aprs_is.reportingDistance = 30;

    loramodule.iGateFreq = 433775000;
    loramodule.digirepeaterTxFreq = 433775000;
    loramodule.digirepeaterRxFreq = 433900000;
    loramodule.spreadingFactor = 12;
    loramodule.signalBandwidth = 125000;
    loramodule.codingRate4 = 5;
    loramodule.power = 20;

    display.alwaysOn = true;
    display.timeout = 4;
    display.turn180 = false;

    syslog.active = false;
    syslog.server = "192.168.0.100";
    syslog.port = 514;

    bme.active = false;

    ota.username = "";
    ota.password = "";

    beaconInterval = 15;
    igateSendsLoRaBeacons = false;
    igateRepeatsLoRaPackets = false;
    rememberStationTime = 30;
    sendBatteryVoltage = false;
    externalVoltageMeasurement = false;
    externalVoltagePin = 34;

    Serial.println("todo escrito");
}

Configuration::Configuration() {
    if (!SPIFFS.begin(false)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("montado");
    }
    bool exists = SPIFFS.exists("/igate_conf.json");
    if (!exists) {
        init();
        writeFile();
        ESP.restart();
    }
    readFile();
}