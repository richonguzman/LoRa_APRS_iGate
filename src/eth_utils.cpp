#include <ETH.h>
#include <SPI.h>

#include "configuration.h"
#include "display.h"

extern Configuration    Config;
extern bool             backUpDigiMode;
extern uint32_t         lastBackupDigiTime;

uint32_t    previousEthMillis  = 0;
uint8_t     EthCounter         = 0;

bool EthLink            = false;
bool EthGotIP           = false;
bool EthConnected       = false;


namespace ETH_Utils {

    void EthEvent(WiFiEvent_t event) {
        String hostname = "iGate-" + Config.callsign;
        switch (event) {
            case ARDUINO_EVENT_ETH_START:
                ETH.setHostname(hostname.c_str());
                break;
                
            case ARDUINO_EVENT_ETH_CONNECTED:
                EthLink = true;
                break;

            case ARDUINO_EVENT_ETH_GOT_IP:
                EthGotIP = true;
                EthConnected = true;
                break;

            case ARDUINO_EVENT_ETH_DISCONNECTED:
                EthConnected = false;
                break;

            case ARDUINO_EVENT_ETH_STOP:
                EthLink = false;
                EthGotIP = false;
                EthConnected = false;
                break;

            default:
                break;
        }

    }

    void checkETH() {
        if (!Config.digi.ecoMode) {
            if (backUpDigiMode) {
                uint32_t EthCheck = millis() - lastBackupDigiTime;
                if (!EthConnected && EthCheck >= 15 * 60 * 1000) {
                    Serial.println("*** Stopping BackUp Digi Mode ***");
                    backUpDigiMode = false;
                } else if (EthConnected) {
                    Serial.println("*** LAN Reconnect Success (Stopping Backup Digi Mode) ***");
                    backUpDigiMode = false;
                    EthCounter = 0;
                }
            }

            if (!backUpDigiMode && !EthConnected && ((millis() - previousEthMillis) >= 30 * 1000)) {
                Serial.print(millis());
                Serial.println("Waiting for LAN Reconnect...");
                previousEthMillis = millis();

                if (Config.backupDigiMode) {
                    EthCounter++;
                }
                if (EthCounter >= 2) {
                    Serial.println("*** LAN lost. Starting BackUp Digi Mode ***");
                    backUpDigiMode = true;
                    lastBackupDigiTime = millis();
                }
            }
        }
    }

    void startETH() {
        u_int8_t counter = 0;
        WiFi.onEvent(ETH_Utils::EthEvent);
        displayShow("", "Connecting to LAN:", "", "       ...", 0);
        Serial.print("Connecting to LAN: ");
        ETH.begin();
        while (((EthLink && EthGotIP) == false) || (counter <= 10))                                                
        {
            delay(500);
            #ifdef INTERNAL_LED_PIN
                digitalWrite(INTERNAL_LED_PIN,HIGH);
            #endif
            Serial.print('.');
            delay(500);
            #ifdef INTERNAL_LED_PIN
                digitalWrite(INTERNAL_LED_PIN,LOW);
            #endif
            counter++;
        }
        if (EthLink && EthGotIP) EthConnected = true;
        #ifdef INTERNAL_LED_PIN
                digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        if (EthConnected) {
            Serial.print("\nConnected as ");
            Serial.print(ETH.localIP());
            Serial.print(" / MAC: ");
            Serial.println(ETH.macAddress());
            displayShow("", " LAN connected!!", "" , "     loading ...", 1000);
        } else {
            Serial.println("\nNot connected to LAN!");
            displayShow("", " LAN not connected!", "" , "     loading ...", 1000);
        }        
    }

    void setup() {
        if (Config.ethernet.use_lan && !Config.digi.ecoMode) startETH();
    }
}