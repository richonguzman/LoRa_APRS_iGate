#include <Arduino.h>
#include <LoRa.h>
#include <WiFi.h>
#include <vector>
#include "pins_config.h"
#include "configuration.h"
#include "display.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "aprs_is_utils.h"
#include "gps_utils.h"
#include "station_utils.h"
#include "query_utils.h"
#include "digi_utils.h"
#include "utils.h"

/*#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>*/

#define VERSION   "2023.06.07"

WiFiClient      espClient;
//AsyncWebServer  server(80);
String          ConfigurationFilePath = "/igate_conf.json";
Configuration   Config(ConfigurationFilePath);

uint32_t        lastTxTime          = 0;
static bool     beacon_update       = true;
unsigned long   previousWiFiMillis  = 0;
uint32_t lastRxTxTime               = millis();

int             myWiFiAPIndex       = 0;
int             myWiFiAPSize        = Config.wifiAPs.size();
WiFi_AP         *currentWiFi        = &Config.wifiAPs[myWiFiAPIndex];
bool            statusAfterBoot     = Config.statusAfterBoot;
int             stationMode         = Config.stationMode;

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;

String firstLine, secondLine, thirdLine, fourthLine, iGateBeaconPacket;

void checkReceivedPacket(String packet) {
  bool queryMessage = false;
  String aprsPacket, Sender, AddresseeAndMessage, Addressee, ackMessage, receivedMessage;
  Serial.print("Received Lora Packet   : " + String(packet));
  if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {
    Serial.print("   ---> APRS LoRa Packet!");
    Sender = packet.substring(3,packet.indexOf(">"));
    if (Sender != Config.callsign) {   // avoid listening yourself by digirepeating
      if (stationMode > 1) {           // only answer when stationMode > 1 (with Ham Licence)
        if (packet.indexOf("::") > 10) {    // its a Message!
          AddresseeAndMessage = packet.substring(packet.indexOf("::")+2);  
          Addressee = AddresseeAndMessage.substring(0,AddresseeAndMessage.indexOf(":"));
          Addressee.trim();
          if (Addressee == Config.callsign) {               // its for me!                     
            if (AddresseeAndMessage.indexOf("{")>0) {     // ack?
              ackMessage = "ack" + AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{")+1);
              ackMessage.trim();
              delay(4000);
              Serial.println(ackMessage);
              for(int i = Sender.length(); i < 9; i++) {
                Sender += ' ';
              }
              LoRa_Utils::sendNewPacket("APRS", Config.callsign + ">APLRG1,RFONLY::" + Sender + ":" + ackMessage);
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1, AddresseeAndMessage.indexOf("{"));
            } else {
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
            }
            if (receivedMessage.indexOf("?") == 0) {
              queryMessage = true;
              delay(2000);
              if (!Config.display.alwaysOn) {
                display_toggle(true);
              }
              LoRa_Utils::sendNewPacket("APRS", QUERY_Utils::process(receivedMessage, Sender, "LoRa"));
              lastRxTxTime = millis();
              show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> QUERY",  1000);
            }
          }
        }
      }
      if (!queryMessage) {
        aprsPacket = APRS_IS_Utils::createPacket(packet);
        if (!Config.display.alwaysOn) {
          display_toggle(true);
        }
        lastRxTxTime = millis();
        espClient.write(aprsPacket.c_str());
        Serial.println("   ---> Uploaded to APRS-IS");
        STATION_Utils::updateLastHeard(Sender);
        if (aprsPacket.indexOf("::") >= 10) {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE ----> MESSAGE",  1000);
        } else if (aprsPacket.indexOf(":>") >= 10) {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE ----> NEW STATUS", 1000);
        } else if (aprsPacket.indexOf(":!") >= 10 || aprsPacket.indexOf(":=") >= 10) {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE ----> GPS BEACON", 1000);
        } else {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE ----> ??????????", 1000);
        }
      }
    }    
  } else {
    Serial.println("   ---> LoRa Packet Ignored (first 3 bytes or TCPIP/NOGATE/RFONLY)\n");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setup_display();
  Serial.println("\nStarting iGate: " + Config.callsign + "   Version: " + String(VERSION));
  show_display("   LoRa APRS iGate", "    Richonguzman", "    -- CD2RXU --", "     " VERSION, 4000);
  WIFI_Utils::validateMode(stationMode);
  iGateBeaconPacket = GPS_Utils::generateBeacon();
  LoRa_Utils::setup();

  /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");*/  
  
  firstLine = "LoRa iGate: " + Config.callsign;
  secondLine = "";
  thirdLine = "  LoRa Module Ready";
  fourthLine = "     listening...";
}

void loop() {
  if (stationMode==3 || stationMode==4) {           // DigiRepeater (3 RxFreq=TxFreq / 4 RxFreq!=TxFreq)
    secondLine = "<DigiRepeater Active>";
    uint32_t lastBeaconTx = millis() - lastTxTime;
    if (lastBeaconTx >= Config.beaconInterval*60*1000) {
      beacon_update = true;    
    }
    if (beacon_update) {
      thirdLine = "";
      show_display(firstLine, secondLine, thirdLine, "SENDING iGate BEACON", 0);
      fourthLine = "     listening...";
      Serial.println("---- Sending iGate Beacon ----");
      //Serial.println(iGateBeaconPacket);
      LoRa_Utils::sendNewPacket("APRS",iGateBeaconPacket);
      lastTxTime = millis();
      beacon_update = false;
    }
    show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
    DIGI_Utils::process(LoRa_Utils::receivePacket());
  } else if (stationMode==1 || stationMode==2 ) {   // iGate (1 Only Rx / 2 Rx+Tx)
    unsigned long currentWiFiMillis   = millis();
    if ((WiFi.status() != WL_CONNECTED) && (currentWiFiMillis - previousWiFiMillis >= currentWiFi->checkInterval*1000)) {
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
      uint32_t lastRxTx = millis() - lastRxTxTime;
      if (!Config.display.alwaysOn) {
        if (lastRxTx >= Config.display.timeout*1000) {
          display_toggle(false);
        }
      }
      if (!Config.display.keepLastPacketOnScreen) {
        show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
      }

      uint32_t lastTx = millis() - lastTxTime;
      if (lastTx >= Config.beaconInterval*60*1000) {
        beacon_update = true;    
      }
      if (beacon_update) {
        display_toggle(true);
        Serial.println("---- Sending iGate Beacon ----");
        //Serial.println(iGateBeaconPacket);
        espClient.write((iGateBeaconPacket + "\n").c_str()); 
        lastTxTime = millis();
        lastRxTxTime = millis();
        show_display(firstLine, secondLine, thirdLine, "SENDING iGate BEACON", 1000);
        thirdLine = "";
        fourthLine = "     listening...";
        show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
        beacon_update = false;
      }

      String loraPacket = "";
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        while (LoRa.available()) {
          int inChar = LoRa.read();
          loraPacket += (char)inChar;
        }
        checkReceivedPacket(loraPacket);
      }
      
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
                lastRxTxTime = millis();
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
                  lastRxTxTime = millis();
                  receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
                  show_display(firstLine, secondLine, Sender + " -> " + Addressee, receivedMessage, 1000);
                }
              }
            }
          }        
        }
      }
      if (statusAfterBoot) {
        utils::processStatus();
      }
    }
  } else {
    Serial.println(stationMode); 
    // this mode is only for when iGate loses Wifi and transforms into Digirepeater
  }
}