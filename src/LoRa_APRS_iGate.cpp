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
#include "tnc_utils.h"
#include "display.h"
#include "utils.h"
#include <ElegantOTA.h>
#include "battery_utils.h"

Configuration   Config;
WiFiClient      espClient;

String          versionDate             = "2024.04.20";
uint8_t         myWiFiAPIndex           = 0;
int             myWiFiAPSize            = Config.wifiAPs.size();
WiFi_AP         *currentWiFi            = &Config.wifiAPs[myWiFiAPIndex];

bool            isUpdatingOTA           = false;
bool            statusAfterBoot         = true;
bool            beaconUpdate            = true;
uint32_t        lastBeaconTx            = 0;
uint32_t        previousWiFiMillis      = 0;
uint32_t        lastScreenOn            = millis();

uint32_t        lastWiFiCheck           = 0;
bool            WiFiConnect             = true;
bool            WiFiConnected           = false;

bool            WiFiAutoAPStarted       = false;
long            WiFiAutoAPTime          = false;

uint32_t        lastBatteryCheck        = 0;

uint32_t        bmeLastReading          = -60000;

String          batteryVoltage;

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;
std::vector<String> outputPacketBuffer;
uint32_t        lastTxTime              = millis();
uint32_t        lastRxTime              = millis();

std::vector<ReceivedPacket> receivedPackets;

String firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, iGateBeaconPacket, iGateLoRaBeaconPacket;

void setup() {
    Serial.begin(115200);

    #ifdef BATTERY_PIN
    pinMode(BATTERY_PIN, INPUT);
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

    LoRa_Utils::setup();
    Utils::validateFreqs();

    iGateBeaconPacket = GPS_Utils::generateBeacon();
    iGateLoRaBeaconPacket = GPS_Utils::generateiGateLoRaBeacon();

#ifdef HELTEC_HTCT62
    if (Config.lowPowerMode) {
        gpio_wakeup_enable(GPIO_NUM_3, GPIO_INTR_HIGH_LEVEL);
        esp_deep_sleep_enable_gpio_wakeup(GPIO_NUM_3, ESP_GPIO_WAKEUP_GPIO_HIGH);

        long lastBeacon = 0;

        LoRa_Utils::startReceive();

        while (true) {
            auto wakeup_reason = esp_sleep_get_wakeup_cause();

            if (wakeup_reason == 7) { // packet received
                Serial.println("Received packet");

                String packet = LoRa_Utils::receivePacket();

                Serial.println(packet);

                if (Config.digi.mode == 2) { // If Digi enabled
                    DIGI_Utils::loop(packet); // Send received packet to Digi
                }

                if (packet.indexOf(Config.callsign + ":?APRSELP{") != -1) { // Send `?APRSELP` to exit low power
                    Serial.println("Got ?APRSELP message, exiting from low power mode");
                    break;
                };
            }

            long time = esp_timer_get_time() / 1000000;

            if (lastBeacon == 0 || time - lastBeacon >= Config.beacon.interval * 60) {
                Serial.println("Sending beacon");

                String comment = Config.beacon.comment;

                if (Config.sendBatteryVoltage) {
                    comment += " Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V";
                }

                if (Config.externalVoltageMeasurement) {
                    comment += " Ext=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V";
                }

                LoRa_Utils::sendNewPacket("APRS", iGateLoRaBeaconPacket + comment);
            
                lastBeacon = time;
            }

            LoRa_Utils::startReceive();

            Serial.println("Sleeping");

            long sleep = (Config.beacon.interval * 60) - (time - lastBeacon);

            Serial.flush();

            esp_sleep_enable_timer_wakeup(sleep * 1000000);
            esp_light_sleep_start();

            Serial.println("Waked up");
        }

        Config.loramodule.rxActive = false;
    }
#endif

    WIFI_Utils::setup();

    SYSLOG_Utils::setup();
    BME_Utils::setup();
    WEB_Utils::setup();
    TNC_Utils::setup();
}

void loop() {
    WIFI_Utils::checkIfAutoAPShouldPowerOff();

    if (isUpdatingOTA) {
        ElegantOTA.loop();
        return; // Don't process IGate and Digi during OTA update
    }

    if (Config.lowVoltageCutOff > 0) {
        BATTERY_Utils::checkIfShouldSleep();
    }

    thirdLine = Utils::getLocalIP();

    WIFI_Utils::checkWiFi(); // Always use WiFi, not related to IGate/Digi mode
    // Utils::checkWiFiInterval();

    if (Config.aprs_is.active && !espClient.connected()) {
        APRS_IS_Utils::connect();
    }

    TNC_Utils::loop();

    Utils::checkDisplayInterval();
    Utils::checkBeaconInterval();

    
    
    APRS_IS_Utils::checkStatus(); // Need that to update display, maybe split this and send APRSIS status to display func?

    String packet = "";
    if (Config.loramodule.rxActive) {
        packet = LoRa_Utils::receivePacket(); // We need to fetch LoRa packet above APRSIS and Digi
    } 

    if (packet != "") {
        if (Config.aprs_is.active) { // If APRSIS enabled
            APRS_IS_Utils::processLoRaPacket(packet); // Send received packet to APRSIS
        }

        if (Config.digi.mode == 2) { // If Digi enabled
            DIGI_Utils::processLoRaPacket(packet); // Send received packet to Digi
        }

        if (Config.tnc.enableServer) { // If TNC server enabled
            TNC_Utils::sendToClients(packet); // Send received packet to TNC KISS
        }

        if (Config.tnc.enableSerial) { // If Serial KISS enabled
            TNC_Utils::sendToSerial(packet); // Send received packet to Serial KISS
        }
    }

    if (Config.aprs_is.active) { // If APRSIS enabled
        APRS_IS_Utils::listenAPRSIS(); // listen received packet from APRSIS
    }

    STATION_Utils::processOutputPacketBuffer();

    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
}