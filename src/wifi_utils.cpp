#include <WiFi.h>
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


namespace WIFI_Utils {

    void checkWiFi() {
        if (!Config.digi.ecoMode) {
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
                WIFI_Utils::startWiFi();//WiFi.reconnect();
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
            while (WiFi.status() != WL_CONNECTED && wifiCounter<myWiFiAPSize) {
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN,HIGH);
                #endif
                Serial.print('.');
                delay(500);
                #ifdef INTERNAL_LED_PIN
                    digitalWrite(INTERNAL_LED_PIN,LOW);
                #endif
                if ((millis() - start) > 10000){
                    delay(1000);
                    if(myWiFiAPIndex >= (myWiFiAPSize - 1)) {
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
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("Connected as ");
            Serial.print(WiFi.localIP());
            Serial.print(" / MAC Address: ");
            Serial.println(WiFi.macAddress());
            displayShow("", "     Connected!!", "" , "     loading ...", 1000);
        } else if (WiFi.status() != WL_CONNECTED) {
            startAP = true;

            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", " WiFi Not Connected!", "" , "     loading ...", 1000);
        }
        WiFiConnected = !startAP;
        if (startAP) {
            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            displayShow("", "   Starting Auto AP", " Please connect to it " , "     loading ...", 1000);

            startAutoAP();
        }
    }

    void checkAutoAPTimeout() {
        if (WiFiAutoAPStarted && Config.wifiAutoAP.timeout > 0) {
            if (WiFi.softAPgetStationNum() > 0) {
                WiFiAutoAPTime = 0;
            } else {
                if (WiFiAutoAPTime == 0) {
                    WiFiAutoAPTime = millis();
                } else if ((millis() - WiFiAutoAPTime) > Config.wifiAutoAP.timeout * 60 * 1000) {
                    Serial.println("Stopping auto AP");

                    WiFiAutoAPStarted = false;
                    WiFi.softAPdisconnect(true);

                    Serial.println("Auto AP stopped (timeout)");
                }
            }
        }
    }

    void setup() {
        if (!Config.digi.ecoMode) startWiFi();
        btStop();
    }

}