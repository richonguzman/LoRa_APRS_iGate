#ifndef IGATE_CONFIG_H_
#define IGATE_CONFIG_H_

#include <Arduino.h>

#define VERSION "V.0.0.1" //MEGA BETA

#define BeaconInterval      900000              // 15 minutes = 900000 seg
#define WifiCheckInterval   60000               // wificheck after one minute

const String AprsSoftwareName       = "ESP32_LoRa_iGate";
const String AprsSoftwareVersion    = "0.0.9";

const String iGateCallsign          = "CD2RXU-11";              // use your own iGate Callsign
const int AprsReportingDistance     = 20;                       // kms
const String AprsFilter             = "t/m/" + iGateCallsign + "/" + (String)AprsReportingDistance;

const String Latitude               = "3302.02S";               // write your own iGate latitude and longitude
const String Longitude              = "07134.42W"; 

#endif