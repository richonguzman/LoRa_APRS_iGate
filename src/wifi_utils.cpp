#include <WiFi.h>
#include "configuration.h"
#include "boards_pinout.h"
#include "wifi_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration  Config;

//extern WiFi_AP        *currentWiFi;
//extern uint8_t        myWiFiAPIndex;
//extern int            myWiFiAPSize;
//extern uint32_t       previousWiFiMillis;

uint8_t     myWiFiAPIndex       = 0;
int         myWiFiAPSize        = Config.wifiAPs.size();
WiFi_AP     *currentWiFi        = &Config.wifiAPs[myWiFiAPIndex];

uint8_t     wifiCounter         = 0;
bool        WiFiConnected       = false;
uint32_t    WiFiAutoAPTime      = millis();
bool        WiFiAutoAPStarted   = false;
uint32_t    previousWiFiMillis  = 0;

namespace WIFI_Utils {

    void checkWiFi() {
        if ((WiFi.status() != WL_CONNECTED) && ((millis() - previousWiFiMillis) >= 30 * 1000) && !WiFiAutoAPStarted) {
            Serial.print(millis());
            Serial.println("Reconnecting to WiFi...");
            WiFi.disconnect();
            WiFi.reconnect();
            previousWiFiMillis = millis();
            wifiCounter++;
        } else if (WiFi.status() == WL_CONNECTED) {
            wifiCounter = 0;
            Serial.println(wifiCounter);
        }
    }

    /*void checkWiFiInterval() {
        uint32_t WiFiCheck = millis() - lastWiFiCheck;
        if (WiFi.status() != WL_CONNECTED && WiFiCheck >= 15 * 60 * 1000) {
            WiFiConnect = true;
        }
        if (WiFiConnect) {
            Serial.println("\nConnecting to WiFi ...");
            WIFI_Utils::startWiFi();
            lastWiFiCheck = millis();
            WiFiConnect = false;
        }
    }*/


    void startAutoAP() {
        WiFi.mode(WIFI_MODE_NULL);

        WiFi.mode(WIFI_AP);
        WiFi.softAP(Config.callsign + " AP", Config.wifiAutoAP.password);

        WiFiAutoAPTime = millis();
        WiFiAutoAPStarted = true;
    }

    void startWiFi() {
        bool startAP = false;
        if (currentWiFi->ssid == "") {
            startAP = true;
        } else {
            uint8_t wifiCounter = 0;
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            delay(500);
            unsigned long start = millis();
            show_display("", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
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
                    show_display("", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
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
            Serial.println(WiFi.localIP());
            show_display("", "     Connected!!", "" , "     loading ...", 1000);
        } else if (WiFi.status() != WL_CONNECTED) {
            startAP = true;

            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            show_display("", " WiFi Not Connected!", "" , "     loading ...", 1000);
        }
        WiFiConnected = !startAP;
        if (startAP) {
            Serial.println("\nNot connected to WiFi! Starting Auto AP");
            show_display("", "   Starting Auto AP", " Please connect to it " , "     loading ...", 1000);

            startAutoAP();
        }
    }

    void checkIfAutoAPShouldPowerOff() {
        if (WiFiAutoAPStarted && Config.wifiAutoAP.powerOff > 0) {
            if (WiFi.softAPgetStationNum() > 0) {
                WiFiAutoAPTime = 0;
            } else {
                if (WiFiAutoAPTime == 0) {
                    WiFiAutoAPTime = millis();
                } else if ((millis() - WiFiAutoAPTime) > Config.wifiAutoAP.powerOff * 60 * 1000) {
                    Serial.println("Stopping auto AP");

                    WiFiAutoAPStarted = false;
                    WiFi.softAPdisconnect(true);

                    Serial.println("Auto AP stopped (timeout)");
                }
            }
        }
    }

    void setup() {
        startWiFi();
        btStop();
    }

}