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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <Arduino.h>
#include <vector>
#include <FS.h>


class WiFi_AP {
public:
    String  ssid;
    String  password;
};

class WiFi_Auto_AP {
public:
    String  password;
    int     timeout;
};

class BEACON {
public:
    double  latitude;
    double  longitude;
    String  comment;
    int     interval;
    String  overlay;
    String  symbol;
    String  path;    
    bool    sendViaRF;
    bool    sendViaAPRSIS;
    bool    gpsActive;
    bool    gpsAmbiguity;
    bool    statusActive;
    String  statusPacket;
};

class APRS_IS {
public:
    bool    active;
    String  passcode;
    String  server;
    int     port;
    String  filter;
    bool    messagesToRF;
    bool    objectsToRF;
};

class DIGI {
public:
    int     mode;
    int     ecoMode;        // 0 = Not Active | 1 = Ultra EcoMode | 2 = Not Active (WiFi OFF, Serial ON)
};

class LoraModule {
public:
    long    txFreq;
    long    rxFreq;
    bool    txActive;
    bool    rxActive;
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

class BATTERY {
public:
    bool    sendInternalVoltage;
    bool    monitorInternalVoltage;
    float   internalSleepVoltage;
    bool    sendExternalVoltage;
    int     externalVoltagePin;
    bool    monitorExternalVoltage;
    float   externalSleepVoltage;
    float   voltageDividerR1;
    float   voltageDividerR2;
    bool    sendVoltageAsTelemetry;
};

class WXSENSOR {
public:
    bool    active;
    int     heightCorrection;
    float   temperatureCorrection;
};

class SYSLOG {
public:
    bool    active;
    String  server;
    int     port;
};

class TNC {
public:
    bool    enableServer;
    bool    enableSerial;
    bool    acceptOwn;
};

class OTA {
public:
    String  username;
    String  password;
};

class WEBADMIN {
public:
    bool    active;
    String  username;
    String  password;
};

class NTP {
public:
    float   gmtCorrection;
};

class REMOTE_MANAGEMENT {
public:
    String  managers;
    bool    rfOnly;
};

class Configuration {
public:
    String                  callsign;
    int                     rememberStationTime;
    bool                    backupDigiMode;
    bool                    rebootMode;
    int                     rebootModeTime;
    String                  personalNote;
    String                  blacklist;
    std::vector<WiFi_AP>    wifiAPs;
    WiFi_Auto_AP            wifiAutoAP;
    BEACON                  beacon;
    APRS_IS                 aprs_is;
    DIGI                    digi;
    LoraModule              loramodule;
    Display                 display;
    BATTERY                 battery;
    WXSENSOR                wxsensor;
    SYSLOG                  syslog;
    TNC                     tnc;  
    OTA                     ota;
    WEBADMIN                webadmin;
    NTP                     ntp;    
    REMOTE_MANAGEMENT       remoteManagement;

    void init();
    void writeFile();
    Configuration();

private:
    bool readFile();
    String _filePath;
};

#endif