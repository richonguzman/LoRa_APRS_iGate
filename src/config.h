#ifndef IGATE_CONFIG_H_
#define IGATE_CONFIG_H_

#include <Arduino.h>

#define WIFI_SSID       "Richon"
#define WIFI_PASSWORD   "k4fPnmg5qnyf"

#define BeaconInterval 900000       // 15 minutes = 900000 seg
#define WifiCheckInterval 60000     // wificheck after one minute

const String WeatherReportCallsign  = "WRCLP";
const String WeatherReportPasscode  = "14332";
const String AprsServer             = "radioaficion.pro";       // write the address of the aprs server   //const String SERVER = "brazil.aprs2.net";
const int AprsServerPort            = 14580;                    // 14579 port is allready filtered so use 14580
const String AprsSoftwareName       = "ESP32_TEST";
const String AprsSoftwareVersion    = "0.1.0";
const int ReportingDistance         = 30;                       // kms
const String AprsFilter             = "t/poms/CD2RXU-10/50";    //cambio a : "t/poms/" + WeatherReportCallsign + "/" + String(ReportingDistance)

const String WeatherReportComment   = "LoRa APRS Weather Report https://github.com/richonguzman/ESP32_APRS_Weather_Report";

const String LAT                    = "3302.04S";      // por corregir               // write your own latitude!!!!
const String LON                    = "07134.43W";     // por corregir   

String WeatherReportBeaconPacket    = WeatherReportCallsign + ">APRS,TCPIP*,qAC,CHILE:=" + LAT + "/" + LON + "?" +  WeatherReportComment + "\n";

const char* ntpServer               = "pool.ntp.org";
const long GMT                      = -3;                // GMT -3
const long  gmtOffset_sec           = GMT*60*60;
const int   daylightOffset_sec      = 3600;


#endif