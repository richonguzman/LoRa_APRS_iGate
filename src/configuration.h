#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <Arduino.h>
#include <vector>
#include <FS.h>

class WiFi_AP {
public:
    String  ssid;
    String  password;
    // double  latitude; // deprecated
    // double  longitude; // deprecated
};

class WiFi_Auto_AP {
public:
    String  password;
    int     powerOff;
};

class Beacon {
public:
    double  latitude; // new
    double  longitude; // new
    String  comment; // new
    String  overlay; // new
    String  symbol; // new
    int     interval; // new
    bool    sendViaRF; // new
    bool    sendViaAPRSIS; // new
    String  path; // new
};

class DIGI {
public:
    int     mode; // new
    // String  comment; // deprecated
    // double  latitude; // deprecated
    // double  longitude; // deprecated
};

class APRS_IS {
public:
    bool    active; // new
    String  passcode;
    String  server;
    int     port;
    // int     reportingDistance; // deprecated
    String  filter; // new
    //bool    toRF; // new
    bool    messagesToRF;
    bool    objectsToRF;
};

class LoraModule {
public:
    // long    iGateFreq; // deprecated
    // long    digirepeaterTxFreq; // deprecated
    // long    digirepeaterRxFreq; // deprecated
    long    txFreq; // new
    long    rxFreq; // new
    bool    txActive; // new
    bool    rxActive; // new
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

class TNC {
public:
    bool    enableServer;
    bool    enableSerial;
    bool    acceptOwn;
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
    int     heightCorrection;
    float   temperatureCorrection;
};

class OTA {
public:
    String  username;
    String  password;
};




class Configuration {
public:
    bool                    reload;
    String                  callsign;  
    // int                   stationMode; // deprecated
    // String                iGateComment; // deprecated
    // int                   beaconInterval; // deprecated
    // bool                  igateSendsLoRaBeacons; // deprecated
    // bool                  igateRepeatsLoRaPackets; // deprecated
    int                     rememberStationTime;
    bool                    sendBatteryVoltage;
    bool                    externalVoltageMeasurement;
    int                     externalVoltagePin;
    bool                    lowPowerMode;
    double                  lowVoltageCutOff;
    bool                    backupDigiMode;
    bool                    rebootMode;
    int                     rebootModeTime;
    std::vector<WiFi_AP>    wifiAPs;
    WiFi_Auto_AP            wifiAutoAP;
    Beacon                  beacon; // new
    DIGI                    digi;
    TNC                     tnc; // new
    APRS_IS                 aprs_is;
    LoraModule              loramodule;
    Display                 display;
    SYSLOG                  syslog;
    BME                     bme;
    OTA                     ota;
  
    void init();
    void writeFile();
    void check();
    Configuration();

private:
    bool readFile();
    String _filePath;
};

#endif