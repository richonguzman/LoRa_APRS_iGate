#include <Arduino.h>
#include <LoRa.h>
#include <WiFi.h>
#include <vector>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "pins_config.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "display.h"
#include "utils.h"
/*#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>*/

#define VERSION   "2023.06.08"

Configuration   Config;
WiFiClient      espClient;
//AsyncWebServer  server(80);

int             myWiFiAPIndex       = 0;
int             myWiFiAPSize        = Config.wifiAPs.size();
WiFi_AP         *currentWiFi        = &Config.wifiAPs[myWiFiAPIndex];
bool            statusAfterBoot     = Config.statusAfterBoot;
int             stationMode         = Config.stationMode;

bool            beacon_update       = true;
uint32_t        lastBeaconTx        = 0;
unsigned long   previousWiFiMillis  = 0;
uint32_t        lastScreenOn        = millis();

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;

String firstLine, secondLine, thirdLine, fourthLine, iGateBeaconPacket;

void setup() {
  Serial.begin(115200);
  delay(1000);
  utils::setupDiplay();
  Serial.println("\nStarting iGate: " + Config.callsign + "   Version: " + String(VERSION));
  show_display("   LoRa APRS iGate", "    Richonguzman", "    -- CD2RXU --", "     " VERSION, 4000);
  WIFI_Utils::setup();
  LoRa_Utils::setup();
  utils::validateDigiFreqs();
  iGateBeaconPacket = GPS_Utils::generateBeacon();

  /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");*/  
}

void loop() {
  if (stationMode==3 || stationMode==4) {           // DigiRepeater (3 RxFreq=TxFreq / 4 RxFreq!=TxFreq)
    utils::checkDisplayInterval();
    utils::checkBeaconInterval();
    show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
    DIGI_Utils::processPacket(LoRa_Utils::receivePacket());
    /*if (statusAfterBoot) {
      utils::processStatus();
    }*/
  } else if (stationMode==1 || stationMode==2 ) {   // iGate (1 Only Rx / 2 Rx+Tx)
    unsigned long currentWiFiMillis   = millis();
    if ((WiFi.status() != WL_CONNECTED) && (currentWiFiMillis - previousWiFiMillis >= 30*1000)) {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      previousWiFiMillis = currentWiFiMillis;
    }
    if (!espClient.connected()) {
      APRS_IS_Utils::connect();
    }
    secondLine  = APRS_IS_Utils::checkStatus();
    show_display(firstLine, secondLine, thirdLine, fourthLine, 0);

    while (espClient.connected()) {
      utils::checkDisplayInterval();
      utils::checkBeaconInterval();
      APRS_IS_Utils::processLoRaPacket(LoRa_Utils::receivePacket());            
      if (espClient.available()) {
        String aprsisData, aprsisPacket, Sender, AddresseeAndMessage, Addressee, receivedMessage;
        bool validHeardStation = false;
        aprsisData = espClient.readStringUntil('\r'); // or '\n'
        aprsisPacket.concat(aprsisData);
        if (!aprsisPacket.startsWith("#")){
          if (aprsisPacket.indexOf("::")>0) {
            Sender = aprsisPacket.substring(0,aprsisPacket.indexOf(">"));
            AddresseeAndMessage = aprsisPacket.substring(aprsisPacket.indexOf("::")+2);
            Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
            Addressee.trim();
            if (Addressee == Config.callsign) {             // its for me!
              if (AddresseeAndMessage.indexOf("{")>0) {     // ack?
                String ackMessage = "ack" + AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{")+1);
                ackMessage.trim();
                delay(4000);
                Serial.println(ackMessage);
                for(int i = Sender.length(); i < 9; i++) {
                  Sender += ' ';
                }
                String ackPacket = Config.callsign + ">APLRG1,TCPIP,qAC::" + Sender + ":" + ackMessage + "\n";
                espClient.write(ackPacket.c_str());
                receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1, AddresseeAndMessage.indexOf("{"));
              } else {
                receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
              }
              if (receivedMessage.indexOf("?") == 0) {
                Serial.println("Received Query APRS-IS : " + aprsisPacket);
                String queryAnswer = QUERY_Utils::process(receivedMessage, Sender, "APRSIS");
                Serial.println("---> QUERY Answer : " + queryAnswer.substring(0,queryAnswer.indexOf("\n")));
                if (!Config.display.alwaysOn) {
                  display_toggle(true);
                }
                lastScreenOn = millis();
                delay(500);
                espClient.write(queryAnswer.c_str());
                show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> QUERY",  1000);
              }
            } else {
              Serial.print("Received from APRS-IS  : " + aprsisPacket);
              if (stationMode == 1) {
                Serial.println("   ---> Cant Tx without Ham Lincence");
              } else if (stationMode > 1) {
                if (STATION_Utils::wasHeard(Addressee)) {
                  LoRa_Utils::sendNewPacket("APRS", LoRa_Utils::generatePacket(aprsisPacket));
                  display_toggle(true);
                  lastScreenOn = millis();
                  receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
                  show_display(firstLine, secondLine, Sender + " -> " + Addressee, receivedMessage, 1000);
                }
              }
            }
          }        
        }
      }
      /*if (statusAfterBoot) {
        utils::processStatus();
      }*/
    }
  }
}