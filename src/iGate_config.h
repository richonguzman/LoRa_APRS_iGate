#ifndef IGATE_CONFIG_H_
#define IGATE_CONFIG_H_

#include <Arduino.h>

#define VERSION "V.0.0.1" //MEGA BETA

#define WIFI_SSID "Richon"
#define WIFI_PASSWORD "k4fPnmg5qnyf"

#define BeaconInterval      900000              // 15 minutes = 900000 seg
#define WifiCheckInterval   60000               // wificheck after one minute

#define EcoModeDisplayTime  5000               // after 5 segs Display goes off

const String iGateCallsign          = "CD2RXU-11";              // use your own iGate Callsign
const String iGatePasscode          = "23201";                  // use your one iGate Callsign Passcode
const String AprsServer             = "radioaficion.pro";       // write the address of the aprs server near you , like "brazil.aprs2.net";
const int AprsServerPort            = 14580;
const String AprsSoftwareName       = "ESP32_LoRa_iGate";
const String AprsSoftwareVersion    = "0.0.9";
const int AprsReportingDistance     = 20;                       // kms
const String AprsFilter             = "t/m/" + iGateCallsign + "/" + (String)AprsReportingDistance;

const String iGateComment           = "LoRa_APRS_iGate https://github.com/richonguzman/LoRa_APRS_iGate";

const String Latitude               = "3302.02S";               // write your own iGate latitude and longitude
const String Longitude              = "07134.42W"; 

String iGateBeaconPacket            = iGateCallsign + ">APRS,TCPIP*,qAC,CHILE:=" + Latitude + "L" + Longitude+ "&" +  iGateComment + "\n";

#endif