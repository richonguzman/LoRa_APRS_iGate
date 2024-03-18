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
#include "web_utils.h"
#include "display.h"
#include "utils.h"
#include <ElegantOTA.h>


Configuration   Config;
WiFiClient      espClient;

String          versionDate           = "2024.03.18";
int             myWiFiAPIndex         = 0;
int             myWiFiAPSize          = Config.wifiAPs.size();
WiFi_AP         *currentWiFi          = &Config.wifiAPs[myWiFiAPIndex];

bool            isUpdatingOTA         = false;
bool            statusAfterBoot       = true;
bool            beaconUpdate          = true;
uint32_t        lastBeaconTx          = 0;
uint32_t        previousWiFiMillis    = 0;
uint32_t        lastScreenOn          = millis();

uint32_t        lastWiFiCheck         = 0;
bool            WiFiConnect           = true;
bool            WiFiConnected         = false;

bool            WiFiAutoAPStarted     = false;
long            WiFiAutoAPTime        = false;

uint32_t        bmeLastReading        = -60000;

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
    #ifdef HAS_INTERNAL_LED
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

    Config.check();

    WIFI_Utils::setup();
    LoRa_Utils::setup();
    Utils::validateFreqs();

    iGateBeaconPacket = GPS_Utils::generateBeacon();
    iGateLoRaBeaconPacket = GPS_Utils::generateiGateLoRaBeacon();

    SYSLOG_Utils::setup();
    BME_Utils::setup();
    WEB_Utils::setup();
}

void loop() {
    WIFI_Utils::checkIfAutoAPShouldPowerOff();

    if (isUpdatingOTA) {
        ElegantOTA.loop();
        return; // Don't process IGate and Digi during OTA update
    }

    thirdLine = Utils::getLocalIP();

    WIFI_Utils::checkWiFi(); // Always use WiFi, not related to IGate/Digi mode
    // Utils::checkWiFiInterval();

    if (Config.aprs_is.active && !espClient.connected()) {
        APRS_IS_Utils::connect();
    }

    Utils::checkDisplayInterval();
    Utils::checkBeaconInterval();

    String packet;
    
    if (Config.loramodule.rxActive) {
        packet = LoRa_Utils::receivePacket(); // We need to fetch LoRa packet above APRSIS and Digi
    }

    APRS_IS_Utils::checkStatus(); // Need that to update display, maybe split this and send APRSIS status to display func?

    if (Config.aprs_is.active) { // If APRSIS enabled
        APRS_IS_Utils::loop(packet); // Send received packet to APRSIS
    }

    if (Config.digi.mode == 2) { // If Digi enabled
        DIGI_Utils::loop(packet); // Send received packet to Digi
    }

    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
}