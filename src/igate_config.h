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

class APRS_IS {
public:
  int     passcode;
  String  server;
  int     port;
  String  software_name;
  String  software_version;
  int     reporting_distance;
};

class LoraModule {
public:
  long frequency;
  int  spreading_factor;
  long signal_bandwidth;
  int  coding_rate4;
  int  power;
};

class Display {
public:
  bool always_on;
  int  timeout;
};

class Configuration {
public:

  String callsign;  
  String comment;
  int beacon_interval;
  std::vector<WiFi_AP> wifiAPs;
  APRS_IS aprs_is;
  LoraModule loramodule;
  Display display;

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
    StaticJsonDocument<1024> data;
    File configFile = fs.open(fileName, "r");
    DeserializationError error = deserializeJson(data, configFile);
    if (error) {
      Serial.println("Failed to read file, using default configuration");
    }

    JsonArray WiFiArray = data["wifi"]["AP"];
    for (int i = 0; i < WiFiArray.size(); i++) {
      WiFi_AP wifiap;
      wifiap.ssid        = WiFiArray[i]["SSID"].as<String>();
      wifiap.password    = WiFiArray[i]["Password"].as<String>();
      wifiap.latitude    = WiFiArray[i]["Latitude"].as<double>();
      wifiap.longitude   = WiFiArray[i]["Longitude"].as<double>();
     
      wifiAPs.push_back(wifiap);
    }

    callsign                      = data["callsign"].as<String>();
    comment                       = data["comment"].as<String>();
    beacon_interval               = data["beacon_interval"].as<int>();
    
    aprs_is.passcode              = data["aprs_is"]["passcode"].as<int>();
    aprs_is.server                = data["aprs_is"]["server"].as<String>();
    aprs_is.port                  = data["aprs_is"]["port"].as<int>();
    aprs_is.software_name         = data["aprs_is"]["software_name"].as<String>();
    aprs_is.software_version      = data["aprs_is"]["software_version"].as<String>();
    aprs_is.reporting_distance    = data["aprs_is"]["reporting_distance"].as<int>();
		
    loramodule.frequency          = data["lora"]["frequency"].as<long>();
    loramodule.spreading_factor   = data["lora"]["spreading_factor"].as<int>();
    loramodule.signal_bandwidth   = data["lora"]["signal_bandwidth"].as<long>();
    loramodule.coding_rate4       = data["lora"]["coding_rate4"].as<int>();
    loramodule.power              = data["lora"]["power"].as<int>();

    display.always_on             = data["display"]["always_on"].as<bool>();
    display.timeout               = data["display"]["timeout"].as<int>();

    configFile.close();
  }
};
bool defaultStatusAfterBoot       = true;
String defaultStatus              = "https://github.com/richonguzman/LoRa_APRS_iGate";
#endif