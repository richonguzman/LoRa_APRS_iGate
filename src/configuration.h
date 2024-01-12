#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <Arduino.h>
#include <vector>
#include <FS.h>

class WiFi_AP {
public:
  String  ssid;
  String  password;
  double  latitude;
  double  longitude;
};

class DIGI {
public:
  String  comment;
  double  latitude;
  double  longitude;
};

class APRS_IS {
public:
  String  passcode;
  String  server;
  int     port;
  int     reportingDistance;
};

class LoraModule {
public:
  long    iGateFreq;
  long    digirepeaterTxFreq;
  long    digirepeaterRxFreq;
  int     spreadingFactor;
  long    signalBandwidth;
  int     codingRate4;
  int     power;
};

class Display {
public:
  bool    alwaysOn;
  int     timeout;
  bool    turn180;
};

class SYSLOG {
public:
  bool    active;
  String  server;
  int     port;
};

class BME {
public:
  bool    active;
};

class OTA {
public:
  String  username;
  String  password;
};



class Configuration {
public:

  String                callsign;  
  int                   stationMode;
  String                iGateComment;
  int                   beaconInterval;
  bool                  igateSendsLoRaBeacons;
  bool                  igateRepeatsLoRaPackets;
  int                   rememberStationTime;
  bool                  sendBatteryVoltage;
  bool                  externalVoltageMeasurement;
  int                   externalVoltagePin;
  std::vector<WiFi_AP>  wifiAPs;
  DIGI                  digi;
  APRS_IS               aprs_is;
  LoraModule            loramodule;
  Display               display;
  SYSLOG                syslog;
  BME                   bme;
  OTA                   ota;
  

  Configuration();
  void validateConfigFile(String currentBeaconCallsign);

private:
  void readFile(fs::FS &fs, const char *fileName) ;
  String _filePath;
};
#endif