#include <WiFi.h>
#include <HTTPClient.h>
#include "configuration.h"
#include "board_pinout.h"
#include "wifi_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern uint8_t          myWiFiAPIndex;
extern int              myWiFiAPSize;
extern WiFi_AP          *currentWiFi;
extern bool             backUpDigiMode;

bool        WiFiConnected       = false;
uint32_t    WiFiAutoAPTime      = millis();
bool        WiFiAutoAPStarted   = false;
uint32_t    previousWiFiMillis  = 0;
uint8_t     wifiCounter         = 0;
uint32_t    lastBackupDigiTime  = millis();
uint32_t    lastInternetCheck   = 0;
const uint32_t internetCheckInterval = 5 * 60 * 1000; // 5 minuta

bool isInternetAvailable() {
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    int httpCode = http.GET();
    http.end();
    return (httpCode == 204);
}

namespace WIFI_Utils {

    void checkWiFi() {
        if (Config.digi.ecoMode == 0) {

            // Provjera interneta iako je WiFi povezan
            if (!backUpDigiMode && (WiFi.status() == WL_CONNECTED) && ((millis() - lastInternetCheck) >= internetCheckInterval)) {
                lastInternetCheck = millis();
                if (!isInternetAvailable()) {
                    Serial.println("*** Internet NOT available despite WiFi connected! Activating Backup Digi Mode ***");
                    backUpDigiMode = true;
                    lastBackupDigiTime = millis();
                } else {
                    Serial.println("*** Internet available ***");
                }
            }

            if (backUpDigiMode) {
                uint32_t WiFiCheck = millis() - lastBackupDigiTime;
                if (WiFi.status() != WL_CONNECTED && WiFiCheck >= 15 * 60 * 1000) {
                    Serial.println("*** Stopping BackUp Digi Mode ***");
                    backUpDigiMode = false;
                    wifiCounter = 0;
                } else if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("*** WiFi Reconnect Success (Stopping Backup Digi Mode) ***");
                    backUpDigiMode = false;
                    wifiCounter = 0;
                }
            }

            if (!backUpDigiMode && (WiFi.status() != WL_CONNECTED) && ((millis() - previousWiFiMillis) >= 30 * 1000) && !WiFiAutoAPStarted) {
                Serial.print(millis());
                Serial.println("Reconnecting to WiFi...");
                WiFi.disconnect();
                WIFI_Utils::startWiFi();
                previousWiFiMillis = millis();

                if (Config.backupDigiMode) {
                    wifiCounter++;
                }
                if (wifiCounter >= 2) {
                    Serial.println("*** Starting BackUp Digi Mode ***");
                    backUpDigiMode = true;
                    lastBackupDigiTime = millis();
                }
            }
        }
    }

    void startAutoAP() {
        WiFi.mode(WIFI_MODE_NULL);

        WiFi.mode(WIFI_AP);
        WiFi.softAP(Config.callsign + "-AP", Config.wifiAutoAP.password);

        WiFiAutoAPTime = millis();
        WiFiAutoAPStarted = true;
    }

    void startWiFi() {
        bool startAP = false;
        if (currentWiFi->ssid == "") {
            startAP = true;
        } else {
            uint8_t wifiCounter = 0;
            String hostName = "iGATE-" + Config.callsign;
            WiFi.setHostname(hostName.c_str());
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(500);
            unsigned long start = millis();
            displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
            Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
            WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
            while (WiFi.status() != WL_CONNECTED && wifiCounter < myWiFiAPSize) {
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN, HIGH);
                #endif
                Serial.print('.');
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN, LOW);
                #endif
                if ((millis() - start) > 10000){
                    delay(1000);
                    if (myWiFiAPIndex >= (myWiFiAPSize - 1)) {
                        myWiFiAPIndex = 0;
                        wifiCounter++;
                    } else {
                        myWiFiAPIndex++;
                    }
                    wifiCounter++;
                    currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
                    start = millis();
                    Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
                    displayShow("", "Connecting to WiFi:", "", currentWiFi->ssid + " ...", 0);
                    WiFi.disconnect();
                    WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
                }
            }
        }

        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN, LOW);
        #endif

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("Connected as ");
            Serial.print(WiFi.localIP());
            Serial.print(" / MAC Add
