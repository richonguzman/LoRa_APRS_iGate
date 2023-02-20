#ifndef IGATE_CONFIG_H_
#define IGATE_CONFIG_H_

#include <Arduino.h>

#define WIFI_SSID       "Richon"
#define WIFI_PASSWORD   "k4fPnmg5qnyf"

#define BeaconInterval 900000 // 15 minutes = 900000 seg

String iGate_Callsign = "CD2RXU-11";
String passcode_igate = "23201";

const String LAT = "3302.03S";      // por corregir               // write your latitude
const String LON = "07134.42W";     //por corregir   

const String iGate_Comment = "DIY ESP32 - LoRa APRS iGATE https://github.com/richonguzman/LoRa_APRS_iGate";

const String SERVER = "radioaficion.pro";       // write the address of the aprs server   //const String SERVER = "brazil.aprs2.net";
const int APRSPORT = 14580;                     // write the aprs server APRSPORT
// 14579 port is allready filtered


String iGateBeaconPacket = iGate_Callsign + ">APLG01,TCPIP*,qAC,T2BRAZIL:=" + LAT + "L" + LON + "&" +  iGate_Comment + "\n";
//String iGateBeaconPacket = iGate_Callsign + ">APLG01,TCPIP*,qAC,CHILE:=" + LAT + "L" + LON + "&" +  iGate_Comment + "\n";


#endif