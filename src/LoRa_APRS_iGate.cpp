#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "syslog_utils.h"
#include "pins_config.h"
#include "query_utils.h"
#include "power_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "bme_utils.h"
#include "display.h"
#include "utils.h"


Configuration   Config;
WiFiClient      espClient;

String          versionDate           = "2024.01.27";
int             myWiFiAPIndex         = 0;
int             myWiFiAPSize          = Config.wifiAPs.size();
WiFi_AP         *currentWiFi          = &Config.wifiAPs[myWiFiAPIndex];

int             stationMode           = Config.stationMode;
bool            statusAfterBoot       = true;
bool            beaconUpdate          = true;
uint32_t        lastBeaconTx          = 0;
uint32_t        previousWiFiMillis    = 0;
uint32_t        lastScreenOn          = millis();

uint32_t        lastWiFiCheck         = 0;
bool            WiFiConnect           = true;
int             lastStationModeState  = 1;

String          batteryVoltage;

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;
std::vector<String> packetBuffer;
std::vector<String> packetBuffer_temp;

String firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, iGateBeaconPacket, iGateLoRaBeaconPacket;

void setup() {
  Serial.begin(115200);
  #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2)
  pinMode(batteryPin, INPUT);
  #endif
  #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_V3) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
  pinMode(internalLedPin, OUTPUT);
  #endif
  if (Config.externalVoltageMeasurement) {
    pinMode(Config.externalVoltagePin, INPUT);
  }
  #if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
  POWER_Utils::setup();
  #endif
  delay(1000);
  Utils::setupDisplay();
  WIFI_Utils::setup();
  LoRa_Utils::setup();
  Utils::validateDigiFreqs();
  iGateBeaconPacket = GPS_Utils::generateBeacon();
  iGateLoRaBeaconPacket = GPS_Utils::generateiGateLoRaBeacon();
  //Utils::startServer();
  SYSLOG_Utils::setup();
  BME_Utils::setup();
}

void loop() {
  if (stationMode==1 || stationMode==2 ) {          // iGate (1 Only Rx / 2 Rx+Tx)
    WIFI_Utils::checkWiFi();
    if (!espClient.connected()) {
      APRS_IS_Utils::connect();
    }
    APRS_IS_Utils::loop();
  } else if (stationMode==3 || stationMode==4) {    // DigiRepeater (3 RxFreq=TxFreq / 4 RxFreq!=TxFreq)
    DIGI_Utils::loop();
  } else if (stationMode==5) {                      // iGate when WiFi and APRS available , DigiRepeater when not (RxFreq=TxFreq)
    Utils::checkWiFiInterval();
    if (WiFi.status() == WL_CONNECTED) {  // iGate Mode
      thirdLine = Utils::getLocalIP();
      if (!espClient.connected()) {
        APRS_IS_Utils::connect();
      }
      if (lastStationModeState == 1) {
        iGateBeaconPacket = GPS_Utils::generateBeacon();
        lastStationModeState = 0;
        //Utils::startServer();
      }
      APRS_IS_Utils::loop();
    } else {                              // DigiRepeater Mode
      DIGI_Utils::loop();
    }
  }
}