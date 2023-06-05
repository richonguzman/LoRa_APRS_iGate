#ifndef IGATE_CONFIG_H_
#define IGATE_CONFIG_H_

#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <vector>

class WiFi_AP {
public:
  String ssid;
  String password;
  double latitude;
  double longitude;
};

class Network {
public:
  bool    DHCP;
  String  ip;
  String  subnet;
  String  gateway;
  String  dns1;
  String  dns2;
};

class APRS_IS {
public:
  int     passcode;
  String  server;
  int     port;
  String  softwareName;
  String  softwareVersion;
  int     reportingDistance;
};

class LoraModule {
public:
  bool enableTx;
  long frequency;
  int  spreadingFactor;
  long signalBandwidth;
  int  codingRate4;
  int  power;
};

class Display {
public:
  bool alwaysOn;
  bool keepLastPacketOnScreen;
  int  timeout;
};

class Configuration {
public:

  String                callsign;  
  String                comment;
  int                   beaconInterval;
  bool                  statusAfterBoot;
  String                defaultStatus;
  std::vector<WiFi_AP>  wifiAPs;
  Network               network;
  APRS_IS               aprs_is;
  LoraModule            loramodule;
  Display               display;
  
  Configuration(String &filePath) {
    _filePath = filePath;
    if (!SPIFFS.begin(false)) {
      Serial.println("SPIFFS Mount Failed");
      return;
    }
    readFile(SPIFFS, _filePath.c_str());
  }

private:
  String _filePath;

  void readFile(fs::FS &fs, const char *fileName) {
    StaticJsonDocument<1536> data;
    File configFile = fs.open(fileName, "r");
    DeserializationError error    = deserializeJson(data, configFile);
    if (error) {
      Serial.println("Failed to read file, using default configuration");
    }

    JsonArray WiFiArray = data["wifi"]["AP"];
    for (int i = 0; i < WiFiArray.size(); i++) {
      WiFi_AP wifiap;
      wifiap.ssid                 = WiFiArray[i]["SSID"].as<String>();
      wifiap.password             = WiFiArray[i]["Password"].as<String>();
      wifiap.latitude             = WiFiArray[i]["Latitude"].as<double>();
      wifiap.longitude            = WiFiArray[i]["Longitude"].as<double>();
     
      wifiAPs.push_back(wifiap);
    }

    callsign                        = data["callsign"].as<String>();
    comment                         = data["comment"].as<String>();
    beaconInterval                  = data["other"]["beaconInterval"].as<int>();
    statusAfterBoot                 = data["other"]["statusAfterBoot"].as<bool>();
    defaultStatus                   = data["other"]["defaultStatus"].as<String>();

    network.DHCP                    = data["network"]["DHCP"].as<bool>();
    network.ip                      = data["network"]["ip"].as<String>();
    network.subnet                  = data["network"]["subnet"].as<String>();
    network.gateway                 = data["network"]["gateway"].as<String>();
    network.dns1                    = data["network"]["dns1"].as<String>();
    network.dns2                    = data["network"]["dns2"].as<String>();

    aprs_is.passcode                = data["aprs_is"]["passcode"].as<int>();
    aprs_is.server                  = data["aprs_is"]["server"].as<String>();
    aprs_is.port                    = data["aprs_is"]["port"].as<int>();
    aprs_is.softwareName            = data["aprs_is"]["softwareName"].as<String>();
    aprs_is.softwareVersion         = data["aprs_is"]["softwareVersion"].as<String>();
    aprs_is.reportingDistance       = data["aprs_is"]["reportingDistance"].as<int>();
		
    loramodule.enableTx             = data["lora"]["enableTx"].as<bool>();
    loramodule.frequency            = data["lora"]["frequency"].as<long>();
    loramodule.spreadingFactor      = data["lora"]["spreadingFactor"].as<int>();
    loramodule.signalBandwidth      = data["lora"]["signalBandwidth"].as<long>();
    loramodule.codingRate4          = data["lora"]["codingRate4"].as<int>();
    loramodule.power                = data["lora"]["power"].as<int>();

    display.alwaysOn                = data["display"]["alwaysOn"].as<bool>();
    display.keepLastPacketOnScreen  = data["display"]["keepLastPacketOnScreen"].as<bool>();
    display.timeout                 = data["display"]["timeout"].as<int>();

    configFile.close();
  }
};
#endif