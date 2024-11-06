#include "configuration.h"
#include "aprs_is_utils.h"
#include "board_pinout.h"
#include "A7670_utils.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"

#ifdef HAS_A7670
    #define TINY_GSM_MODEM_SIM7600      //The AT instruction of A7670 is compatible with SIM7600 
    #define TINY_GSM_RX_BUFFER 1024     // Set RX buffer to 1Kb
    #define SerialAT Serial1
    #include <TinyGsmClient.h>
    #include <StreamDebugger.h>
    StreamDebugger debugger(SerialAT, Serial);
    TinyGsm modem(debugger);

    extern Configuration    Config;
    extern String           firstLine;

    bool modemStartUp       = false;
    bool serverStartUp      = false;
    bool userBytesSended    = false;
    bool beaconBytesSended  = false;
    bool beaconSended       = false;
    bool stationBeacon      = false;

    extern bool modemLoggedToAPRSIS;


    namespace A7670_Utils {

        bool checkModemOn() {
            bool modemReady = false;
            Serial.print("Starting Modem ...              ");
            displayShow(firstLine, "Starting Modem...", " ", " ", 0);

            pinMode(A7670_ResetPin, OUTPUT);        //A7670 Reset
            digitalWrite(A7670_ResetPin, LOW);
            delay(100);
            digitalWrite(A7670_ResetPin, HIGH);
            delay(3000);
            digitalWrite(A7670_ResetPin, LOW);

            pinMode(A7670_PWR_PIN, OUTPUT);
            digitalWrite(A7670_PWR_PIN, LOW);
            delay(100);
            digitalWrite(A7670_PWR_PIN, HIGH);
            delay(1000);
            digitalWrite(A7670_PWR_PIN, LOW);

            int i = 20;
            while (i) {
                SerialAT.println("AT");
                delay(500);
                if (SerialAT.available()) {
                    String r = SerialAT.readString();
                    //Serial.println(r);
                    if ( r.indexOf("OK") >= 0 ) {
                        modemReady = true;
                        i = 1;
                        Serial.println("Modem Ready!\n");
                        displayShow(firstLine, "Starting Modem...", "---> Modem Ready", " ", 0);
                        return true;
                    }
                }
                if (!modemReady) {
                    delay(500);
                }
                i--;
            }
            return false;
        }

        void setup() {
            SerialAT.begin(115200, SERIAL_8N1, A7670_RX_PIN, A7670_TX_PIN);
            if (checkModemOn()) {;
                delay(1000);
                //setup_gps();      // if gps active / won't be need for now
            } else {
                displayShow(firstLine, "Starting Modem...", "---> Failed !!!", " ", 0);
                Serial.println(F("*********** Failed to connect to the modem! ***********"));
            }
        }

        bool checkATResponse(const String& ATMessage) {
            int delayATMessage = 3000;
            bool validAT = false;
            //Serial.println(ATMessage);
            int i = 10;
            while (i) {
                if (!validAT) {
                    SerialAT.println(ATMessage);
                }
                delay(500);
                if (SerialAT.available()) {
                    String response = SerialAT.readString();
                    //Serial.println(response); // DEBUG of Modem AT message
                    if(response.indexOf("verified") >= 0) {
                        Serial.println("Logged! (User Validated)\n");
                        displayShow(firstLine, "Connecting APRS-IS...", "---> Logged!", " ", 1000);
                        Serial.println("####################   APRS-IS FEED   ####################");
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    } else if (ATMessage == "AT+NETOPEN" && response.indexOf("OK") >= 0) {
                        Serial.println("Port Open!");
                        displayShow(firstLine, "Opening Port...", "---> Port Open", " ", 0);
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    } else if (ATMessage == "AT+NETOPEN" && response.indexOf("Network is already opened") >= 0) {
                        Serial.println("Port Open! (was already opened)");
                        displayShow(firstLine, "Opening Port...", "---> Port Open", " ", 0);
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    } else if (ATMessage.indexOf("AT+CIPOPEN") == 0 && response.indexOf("PB DONE") >= 0) {
                        Serial.println("Contacted!");
                        displayShow(firstLine, "Connecting APRS-IS...", "---> Contacted", " ", 0);
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    } else if (ATMessage.indexOf("AT+CIPSEND=0,") == 0 && response.indexOf(">") >= 0) {
                        Serial.print(".");
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    } else if (ATMessage.indexOf(Config.callsign) >= 3 && !modemLoggedToAPRSIS && response.indexOf("OK") >= 0 && !stationBeacon) { // login info
                        validAT = true;
                        delayATMessage = 0;       
                    } else if (ATMessage.indexOf(Config.callsign) == 0 && !beaconSended && response.indexOf("OK") >= 0 && !stationBeacon) {   // self beacon or querys
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    } else if (stationBeacon && response.indexOf("OK") >= 0) { //upload others beacons
                        validAT = true;
                        i = 1;
                        delayATMessage = 0;
                    }
                }
                delay(delayATMessage);
                i--;
            }
            return validAT;
        }

        void APRS_IS_connect() {
            String loginInfo = "user ";
            loginInfo += Config.callsign;
            loginInfo += " pass ";
            loginInfo += String(Config.aprs_is.passcode);
            loginInfo += " vers CA2RXU_LoRa_iGate 1.3 filter ";
            loginInfo += Config.aprs_is.filter;
            Serial.println("-----> Connecting to APRS IS");
            while (!modemStartUp) {
                Serial.print("Opening Port...                 ");
                displayShow(firstLine, "Opening Port...", " ", " ", 0);
                modemStartUp = checkATResponse("AT+NETOPEN");
                delay(2000);
            } while (!serverStartUp) {
                Serial.print("Connecting APRS-IS Server...    ");
                displayShow(firstLine, "Connecting APRS-IS...", " ", " ", 0);
                serverStartUp = checkATResponse("AT+CIPOPEN=0,\"TCP\",\"" + String(Config.aprs_is.server) + "\"," + String(Config.aprs_is.port));
                delay(2000);
            } while (!userBytesSended) {
                Serial.print("Writing User Login Data       ");
                displayShow(firstLine, "Connecting APRS-IS...", "---> User Login Data", " ", 0);
                userBytesSended = checkATResponse("AT+CIPSEND=0," + String(loginInfo.length()+1));
                delay(2000);
            } while (!modemLoggedToAPRSIS) {
                Serial.print(".");
                modemLoggedToAPRSIS = checkATResponse(loginInfo);
                delay(2000);
            }
        }

        void uploadToAPRSIS(const String& packet) {
            beaconBytesSended = checkATResponse("AT+CIPSEND=0," + String(packet.length()+1));
            delay(2000);
            if (beaconBytesSended) {
                Serial.print(".");
                beaconSended = checkATResponse(packet);
            } 
            if (!beaconSended) {
                Serial.println("------------------------------------> UPLOAD FAILED!!!");
            } else {
                Serial.println("Packet Uploaded to APRS-IS!");
            }
            beaconBytesSended = false;
            beaconSended = false;
        }

        void listenAPRSIS() {
            if (SerialAT.available()) {
                String packetAPRSIS = SerialAT.readStringUntil('\r');
                packetAPRSIS.trim();
                if (!packetAPRSIS.startsWith("#") && packetAPRSIS.indexOf("+IPD") == -1 && packetAPRSIS.indexOf("RECV") == -1 && packetAPRSIS.length() > 0) {
                    APRS_IS_Utils::processAPRSISPacket(packetAPRSIS);
                }
            }
            delay(1);
        }
    }
#endif