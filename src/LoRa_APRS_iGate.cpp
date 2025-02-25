/*___________________________________________________________________

██╗      ██████╗ ██████╗  █████╗      █████╗ ██████╗ ██████╗ ███████╗
██║     ██╔═══██╗██╔══██╗██╔══██╗    ██╔══██╗██╔══██╗██╔══██╗██╔════╝
██║     ██║   ██║██████╔╝███████║    ███████║██████╔╝██████╔╝███████╗
██║     ██║   ██║██╔══██╗██╔══██║    ██╔══██║██╔═══╝ ██╔══██╗╚════██║
███████╗╚██████╔╝██║  ██║██║  ██║    ██║  ██║██║     ██║  ██║███████║
╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝    ╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝╚══════╝
                                                                     
                ██╗ ██████╗  █████╗ ████████╗███████╗
                ██║██╔════╝ ██╔══██╗╚══██╔══╝██╔════╝
                ██║██║  ███╗███████║   ██║   █████╗
                ██║██║   ██║██╔══██║   ██║   ██╔══╝
                ██║╚██████╔╝██║  ██║   ██║   ███████╗
                ╚═╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝


                       Ricardo Guzman - CA2RXU 
          https://github.com/richonguzman/LoRa_APRS_Tracker
             (donations : http://paypal.me/richonguzman)
___________________________________________________________________*/

#include <ElegantOTA.h>
#include <TinyGPS++.h>
#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "configuration.h"
#include "battery_utils.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "query_utils.h"
#include "power_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "web_utils.h"
#include "tnc_utils.h"
#include "ntp_utils.h"
#include "wx_utils.h"
#include "display.h"
#include "utils.h"
#ifdef HAS_A7670
    #include "A7670_utils.h"
#endif

String              versionDate             = "2025.02.25";
Configuration       Config;
WiFiClient          espClient;
#ifdef HAS_GPS
    HardwareSerial  gpsSerial(1);
    TinyGPSPlus     gps;
    uint32_t        gpsSatelliteTime        = 0;
    bool            gpsInfoToggle           = false;
#endif

uint8_t             myWiFiAPIndex           = 0;
int                 myWiFiAPSize            = Config.wifiAPs.size();
WiFi_AP             *currentWiFi            = &Config.wifiAPs[myWiFiAPIndex];

bool                isUpdatingOTA           = false;
uint32_t            lastBatteryCheck        = 0;

bool                backUpDigiMode          = false;
bool                modemLoggedToAPRSIS     = false;

std::vector<ReceivedPacket> receivedPackets;

String firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine;

//#define STARTUP_DELAY 5 //min

void setup() {
    Serial.begin(115200);
    POWER_Utils::setup();
    Utils::setupDisplay();
    LoRa_Utils::setup();
    Utils::validateFreqs();
    GPS_Utils::setup();
    STATION_Utils::loadBlackList();

    #ifdef STARTUP_DELAY    // (TEST) just to wait for WiFi init of Routers
        displayShow("", "  STARTUP DELAY ...", "", "", 0);
        delay(STARTUP_DELAY * 60 * 1000);
    #endif

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
                    if (Config.digi.mode == 2) DIGI_Utils::processLoRaPacket(packet);

                    if (packet.indexOf(Config.callsign + ":?APRSELP{") != -1) { // Send `?APRSELP` to exit low power
                        Serial.println("Got ?APRSELP message, exiting from low power mode");
                        break;
                    };
                }
                long time = esp_timer_get_time() / 1000000;
                if (lastBeacon == 0 || time - lastBeacon >= Config.beacon.interval * 60) {
                    Serial.println("Sending beacon");
                    String comment = Config.beacon.comment;
                    if (Config.battery.sendInternalVoltage) comment += " Batt=" + String(BATTERY_Utils::checkInternalVoltage(),2) + "V";
                    if (Config.battery.sendExternalVoltage) comment += " Ext=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V";
                    LoRa_Utils::sendNewPacket(GPS_Utils::getiGateLoRaBeaconPacket() + comment);
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
    DIGI_Utils::checkEcoMode();
    WIFI_Utils::setup();
    NTP_Utils::setup();
    SYSLOG_Utils::setup();
    WX_Utils::setup();
    WEB_Utils::setup();
    TNC_Utils::setup();
    #ifdef HAS_A7670
        A7670_Utils::setup();
    #endif
    Utils::checkRebootMode();
    APRS_IS_Utils::firstConnection();
}

void loop() {
    WIFI_Utils::checkAutoAPTimeout();

    if (isUpdatingOTA) {
        ElegantOTA.loop();
        return; // Don't process IGate and Digi during OTA update
    }

    if (Config.lowVoltageCutOff > 0) {
        BATTERY_Utils::checkIfShouldSleep();
    }
    
    #ifdef HAS_GPS
        if (Config.beacon.gpsActive) {
            if (millis() - gpsSatelliteTime > 5000) {
                gpsInfoToggle = !gpsInfoToggle;
                gpsSatelliteTime = millis();
            }
            if (gpsInfoToggle) {
                thirdLine = "Satellite(s): ";
                String gpsData = String(gps.satellites.value());
                if (gpsData.length() < 2) gpsData = "0" + gpsData;  // Ensure two-digit formatting
                thirdLine += gpsData;
            } else {
                thirdLine = Utils::getLocalIP();
            }
        } else {
            thirdLine = Utils::getLocalIP();
        }
    #else
        thirdLine = Utils::getLocalIP();
    #endif

    #ifdef HAS_A7670
        if (Config.aprs_is.active && !modemLoggedToAPRSIS) A7670_Utils::APRS_IS_connect();
    #else
        WIFI_Utils::checkWiFi();
        if (Config.aprs_is.active && (WiFi.status() == WL_CONNECTED) && !espClient.connected()) APRS_IS_Utils::connect();
    #endif

    NTP_Utils::update();
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

        if (Config.loramodule.txActive && (Config.digi.mode == 2 || Config.digi.mode == 3 || backUpDigiMode)) { // If Digi enabled
            STATION_Utils::clean25SegBuffer();
            DIGI_Utils::processLoRaPacket(packet); // Send received packet to Digi
        }

        if (Config.tnc.enableServer) { // If TNC server enabled
            TNC_Utils::sendToClients(packet); // Send received packet to TNC KISS
        }
        if (Config.tnc.enableSerial) { // If Serial KISS enabled
            TNC_Utils::sendToSerial(packet); // Send received packet to Serial KISS
        }
    }

    if (Config.aprs_is.active) {
        APRS_IS_Utils::listenAPRSIS(); // listen received packet from APRSIS
    }

    STATION_Utils::processOutputPacketBuffer();

    displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
    Utils::checkRebootTime();
    Utils::checkSleepByLowBatteryVoltage(1);
}