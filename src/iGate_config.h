#ifndef GPS_CONFIG_H_
#define GPS_CONFIG_H_

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
  bool    active;
  int     passcode;
  String  server;
  int     port;

};

class LoRa {
public:
  long frequency;
  int  spreadingFactor;
  long signalBandwidth;
  int  codingRate4;
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
  std::vector<WiFi_AP> wifiAPs;

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

    callsign = data["callsign"].as<String>();
    comment  = data["comment"].as<String>();


    /*conf.aprs_is.active     = data["aprs_is"]["active"];
    conf.aprs_is.passcode   = data["aprs_is"]["passcode"];
    conf.aprs_is.server     = data["aprs_is"]["server"];
    conf.aprs_is.port       = data["aprs_is"]["port"];

    conf.lora.port          = data["lora"]["frequency"];
    conf.lora.port          = data["lora"]["spreading_factor"];
    conf.lora.port          = data["lora"]["signal_bandwidth"];
    conf.lora.port          = data["lora"]["coding_rate4"];
    conf.lora.port          = data["lora"]["power"];
    
    conf.display.always_on  = data["display"]["always_on"];
    conf.display.timeout    = data["display"]["timeout"];*/


    configFile.close();
  }
};
#endif

/*class Configuration {
public:
  /*class WiFiAccessPoint {
  public:
    class WiFiAP {
      WiFiAP(): SSID(), Password(), Latitude(), Longitude() {
      }
      std::string SSID;
      std::string Password;
      long Latitude;
      long Longitude;
    }

    WiFiAccessPoint() : active() {
    }
    bool active;
  };*/

  /*class APRSIS {
  public:
    APRSIS() : active(), passcode(), server(), port() {
    }
    bool    active;
    int     passcode;
    String  server;
    int     port;

  };

  class LoRa {
  public:
    LoRa() : frequency(), power(), spreadingFactor(), signalBandwidth(), codingRate4() {
    }
    long frequency;
    int  spreadingFactor;
    long signalBandwidth;
    int  codingRate4;
    int  power;
  };

  class Display {
  public:
    Display() : always_on(), timeout() {
    }
    bool always_on;
    int  timeout;
  };


  Configuration() : callsign(), comment() {
  }

  String            callsign;
  String            comment;
  /// WIFI
  std::list<Beacon> beacons;
  ///
  WiFiAccessPioint  wifiap;
  APRSIS            aprsis;
  LoRa              lora;
  Display           display;

};*/