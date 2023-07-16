#include <Arduino.h>
#include <LoRa.h>
#include <WiFi.h>
#include <vector>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "syslog_utils.h"
#include "pins_config.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "bme_utils.h"
#include "display.h"
#include "utils.h"


Configuration   Config;
WiFiClient      espClient;

String          versionDate         = "2023.07.12";
int             myWiFiAPIndex       = 0;
int             myWiFiAPSize        = Config.wifiAPs.size();
WiFi_AP         *currentWiFi        = &Config.wifiAPs[myWiFiAPIndex];

int             stationMode         = Config.stationMode;
bool            statusAfterBoot     = true;
bool            beaconUpdate       = true;
uint32_t        lastBeaconTx        = 0;
uint32_t        previousWiFiMillis  = 0;
uint32_t        lastScreenOn        = millis();

String          batteryVoltage;

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;

String firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, iGateBeaconPacket;

void setup() {
  Serial.begin(115200);
  pinMode(batteryPin, INPUT);
  pinMode(greenLed, OUTPUT);
  delay(1000);
  Utils::setupDisplay();
  WIFI_Utils::setup();
  LoRa_Utils::setup();
  Utils::validateDigiFreqs();
  iGateBeaconPacket = GPS_Utils::generateBeacon();
  Utils::startOTAServer();
  SYSLOG_Utils::setup();
  BME_Utils::setup();
}

void loop() {
  if (stationMode==1 || stationMode==2 ) {          // iGate (1 Only Rx / 2 Rx+Tx)
    WIFI_Utils::checkWiFi();
    if (!espClient.connected()) {
      APRS_IS_Utils::connect();
    }
    APRS_IS_Utils::checkStatus();
    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);    
    while (espClient.connected()) {
      Utils::checkDisplayInterval();
      Utils::checkBeaconInterval();
      APRS_IS_Utils::processLoRaPacket(LoRa_Utils::receivePacket());            
      if (espClient.available()) {
        String aprsisPacket;
        aprsisPacket.concat(espClient.readStringUntil('\r'));
        APRS_IS_Utils::processAPRSISPacket(aprsisPacket);
      }
    }
  } else if (stationMode==3 || stationMode==4) {    // DigiRepeater (3 RxFreq=TxFreq / 4 RxFreq!=TxFreq)
    Utils::checkDisplayInterval();
    Utils::checkBeaconInterval();
    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
    DIGI_Utils::processPacket(LoRa_Utils::receivePacket());
  }
}