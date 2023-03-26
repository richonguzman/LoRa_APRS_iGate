#include "igate_config.h"
/*#include "Arduino.h"
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <vector>

/*class WiFi_AP {
public:
  String ssid;
  String password;
  double latitude;
  double longitude;
};

class Configuration {
public:
  //std::vector<WiFi_AP> wifiAPs;

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

    Configuration conf;

    conf.callsign = data["callsign"];
    conf.comment  = data["comment"];

    JsonArray WiFiArray = doc["wifi"]["AP"];
    for (int i = 0; i < WiFiArray.size(); i++) {
      WiFi_AP wifi;
      wifi.ssid        = WiFiArray[i]["wifi"]["AP"]["SSID"].as<String>();
      wifi.password    = WiFiArray[i]["wifi"]["AP"]["Password"].as<String>();
      wifi.latitude    = WiFiArray[i]["wifi"]["AP"]["Latitude"].as<double>();
      wifi.longitude   = WiFiArray[i]["wifi"]["AP"]["Longitude"].as<double>();
     
      conf.wifiAPs.push_back(wifi);
    }

    conf.aprs_is.active     = data["aprs_is"]["active"];
    conf.aprs_is.passcode   = data["aprs_is"]["passcode"];
    conf.aprs_is.server     = data["aprs_is"]["server"];
    conf.aprs_is.port       = data["aprs_is"]["port"];

    conf.lora.port          = data["lora"]["frequency"];
    conf.lora.port          = data["lora"]["spreading_factor"];
    conf.lora.port          = data["lora"]["signal_bandwidth"];
    conf.lora.port          = data["lora"]["coding_rate4"];
    conf.lora.port          = data["lora"]["power"];
    
    conf.display.always_on  = data["display"]["always_on"];
    conf.display.timeout    = data["display"]["timeout"];

    configFile.close();
  }
}*/