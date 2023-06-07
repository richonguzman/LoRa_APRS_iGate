#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <vector>


class WiFi_AP {
public:
  String  ssid;
  String  password;
  double  latitude;
  double  longitude;
  int     checkInterval;
};

class APRS_IS {
public:
  String  passcode;
  String  server;
  int     port;
  String  softwareName;
  String  softwareVersion;
  int     reportingDistance;
};

class LoraModule {
public:
  long    frequencyTx;
  long    frequencyRx;
  int     spreadingFactor;
  long    signalBandwidth;
  int     codingRate4;
  int     power;
};

class Display {
public:
  bool    alwaysOn;
  bool    keepLastPacketOnScreen;
  int     timeout;
};

class Configuration {
public:

  String                callsign;  
  int                   stationMode;
  String                comment;
  int                   beaconInterval;
  int                   rememberStationTime;
  bool                  statusAfterBoot;
  String                defaultStatus;
  std::vector<WiFi_AP>  wifiAPs;
  APRS_IS               aprs_is;
  LoraModule            loramodule;
  Display               display;
  

  Configuration(const String &filePath);
  void validateConfigFile(String currentBeaconCallsign);

private:
  Configuration() {}; // Hide default constructor
  void readFile(fs::FS &fs, const char *fileName) ;
  String _filePath;
};
#endif