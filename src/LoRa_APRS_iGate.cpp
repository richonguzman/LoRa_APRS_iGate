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
#include "utils.h"

/*#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>*/

#define VERSION   "2023.06.06"

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

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;

String firstLine, secondLine, thirdLine, fourthLine, iGateBeaconPacket;

String createAPRSPacket(String unprocessedPacket) {
  String callsign_and_path_tracker, payload_tracker, processedPacket;
  int dotsPosition = unprocessedPacket.indexOf(':');
  callsign_and_path_tracker = unprocessedPacket.substring(3, dotsPosition);
  payload_tracker = unprocessedPacket.substring(dotsPosition);
  if (Config.loramodule.enableTx) {
    processedPacket = callsign_and_path_tracker + ",qAR," + Config.callsign + payload_tracker + "\n";
  } else {
    processedPacket = callsign_and_path_tracker + ",qAO," + Config.callsign + payload_tracker + "\n";
  }
  return processedPacket;
}

bool checkValidHeardStation(String station) {
  bool validStation = false;
  for (int i=0; i<lastHeardStation.size(); i++) {
    if (lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) == station) {
      validStation = true;
      Serial.println(" ---> Listened Station");
    } 
  }
  if (!validStation) {
    Serial.println("   ---> Station not Heard for last 30 min (Not Tx)\n");
  }
  return validStation;
}

void deleteNotHeardStation() {
  uint32_t minReportingTime = 30*60*1000; // 30 minutes // from .json and CONFIGURATION?????
  for (int i=0; i<lastHeardStation.size(); i++) {
    String deltaTimeString = lastHeardStation[i].substring(lastHeardStation[i].indexOf(",")+1);
    uint32_t deltaTime = deltaTimeString.toInt();
    if ((millis() - deltaTime) < minReportingTime) {
      lastHeardStation_temp.push_back(lastHeardStation[i]);
    }
  }
  lastHeardStation.clear();
  for (int j=0; j<lastHeardStation_temp.size(); j++) {
    lastHeardStation.push_back(lastHeardStation_temp[j]);
  }
  lastHeardStation_temp.clear();
}

void updateLastHeardStation(String station) {
  bool stationHeard = false;
  for (int i=0; i<lastHeardStation.size(); i++) {
    if (lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) == station) {
      lastHeardStation[i] = station + "," + String(millis());
      stationHeard = true;
    }
  }
  if (!stationHeard) {
    lastHeardStation.push_back(station + "," + String(millis()));
  }

  //////
  Serial.print("Stations Near (last 30 minutes): ");
  for (int k=0; k<lastHeardStation.size(); k++) {
    Serial.print(lastHeardStation[k].substring(0,lastHeardStation[k].indexOf(","))); Serial.print(" ");
  }
  Serial.println("");
}

String processQueryAnswer(String query, String station, String queryOrigin) {
  String processedQuery, queryAnswer;
  if (query=="?APRS?" || query=="?aprs?" || query=="?Aprs?" || query=="H" || query=="h" || query=="Help" || query=="help" || query=="?") {
    processedQuery = "?APRSV ?APRSP ?APRSL ?APRSH ?WHERE callsign";
  } else if (query=="?APRSV" || query=="?aprsv" || query=="?Aprsv") {
    processedQuery = Config.aprs_is.softwareName + " " + Config.aprs_is.softwareVersion;
  } else if (query=="?APRSP" || query=="?aprsp" || query=="?Aprsp") {
    processedQuery = "iGate QTH: " + String(currentWiFi->latitude) + " " + String(currentWiFi->longitude);
  } else if (query=="?APRSL" || query=="?aprsl" || query=="?Aprsl") {
    for (int i=0; i<lastHeardStation.size(); i++) {
      processedQuery += lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) + " ";
    }
    processedQuery.trim();
  } /*else if (query.indexOf("?APRSH") == 0 || query.indexOf("?aprsv") == 0 || query.indexOf("?Aprsv") == 0) {
     // sacar callsign despues de ?APRSH
    Serial.println("escuchaste a X estacion? en las ultimas 24 o 8 horas?");
    processedQuery = "APRSH";
  } else if (query.indexOf("?WHERE") == 0) { 
    // agregar callsign para completar donde esta X callsign
    Serial.println("estaciones escuchadas directo (ultimos 30 min)");
    processedQuery = "WHERE";
  }*/
  for(int i = station.length(); i < 9; i++) {
    station += ' ';
  }
  if (queryOrigin == "APRSIS") {
    queryAnswer = Config.callsign + ">APLR10,TCPIP,qAC::" + station + ":" + processedQuery + "\n";
  } else if (queryOrigin == "LoRa") {
    queryAnswer = Config.callsign + ">APLR10,RFONLY::" + station + ":" + processedQuery;
  }
  return queryAnswer;
}

void checkReceivedPacket(String packet) {
  bool queryMessage = false;
  String aprsPacket, Sender, AddresseeAndMessage, Addressee, ackMessage, receivedMessage;
  Serial.print("Received Lora Packet   : " + String(packet));
  if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {
    Serial.print("   ---> APRS LoRa Packet!");
    Sender = packet.substring(3,packet.indexOf(">"));
    if (Sender != Config.callsign) {   // avoid listening yourself by digirepeating
      if (Config.loramodule.enableTx) {
        if (packet.indexOf("::") > 10) {    // its a Message!
          AddresseeAndMessage = packet.substring(packet.indexOf("::")+2);  
          Addressee = AddresseeAndMessage.substring(0,AddresseeAndMessage.indexOf(":"));
          Addressee.trim();
          if (Addressee == Config.callsign) {            // its for me!
            if (AddresseeAndMessage.indexOf("{")>0) {    // ack?
              ackMessage = "ack" + AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{")+1);
              ackMessage.trim();
              delay(4000);
              Serial.println(ackMessage);
              for(int i = Sender.length(); i < 9; i++) {
                Sender += ' ';
              }
              LoRaUtils::sendNewPacket("APRS", Config.callsign + ">APLR10,RFONLY::" + Sender + ":" + ackMessage);
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1, AddresseeAndMessage.indexOf("{"));
            } else {
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
            }
            if (receivedMessage.indexOf("?") == 0) {
              queryMessage = true;
              String queryAnswer = processQueryAnswer(receivedMessage, Sender, "LoRa");
              delay(2000);
              if (!Config.display.alwaysOn) {
                display_toggle(true);
              }
              lastRxTxTime = millis();
              LoRaUtils::sendNewPacket("APRS", queryAnswer); 
              show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> QUERY",  1000);
            } 
          }
        }
      }
      if (!queryMessage) {
        aprsPacket = createAPRSPacket(packet);
        if (!Config.display.alwaysOn) {
          display_toggle(true);
        }
        lastRxTxTime = millis();
        espClient.write(aprsPacket.c_str());
        Serial.println("   ---> Uploaded to APRS-IS");
        deleteNotHeardStation();
        updateLastHeardStation(Sender);
        if (aprsPacket.indexOf("::") >= 10) {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> MESSAGE",  1000);
        } else if (aprsPacket.indexOf(":>") >= 10) {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> NEW STATUS", 1000);
        } else if (aprsPacket.indexOf(":!") >= 10) {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> GPS BEACON", 1000);
        } else {
          show_display(firstLine, secondLine, "Callsign = " + Sender, "TYPE --> ??????????", 1000);
        }
      }
    }    
  } else {
    Serial.println("   ---> Not APRS Packet (Ignore)\n");
  }
}

String processAPRSISPacket(String aprsisMessage) {
  String firstPart, messagePart, newLoraPacket;
  aprsisMessage.trim();
  firstPart = aprsisMessage.substring(0, aprsisMessage.indexOf(","));
  messagePart = aprsisMessage.substring(aprsisMessage.indexOf("::")+2);
  newLoraPacket = firstPart + ",TCPIP," + Config.callsign + "::" + messagePart;
  Serial.print("Received from APRS-IS  : " + aprsisMessage);
  return newLoraPacket;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setup_display();
  Serial.println("\nStarting iGate: " + Config.callsign + "   Version: " + String(VERSION));
  show_display("   LoRa APRS iGate", "    Richonguzman", "    -- CD2RXU --", "     " VERSION, 4000); 
  WIFI_Utils::setup();
  btStop();

  /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");*/

  LoRaUtils::setup();
  iGateBeaconPacket = GPS_Utils::generateBeacon();
}

void loop() {
  String wifiState, aprsisState;
  firstLine = "LoRa iGate: " + Config.callsign;
  secondLine = "";
  thirdLine   = "";
  fourthLine  = "";
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

  /*if (WiFi.status() == WL_CONNECTED) {
    wifiState = "OK"; 
  } else {
    wifiState = "--";
    if (!Config.display.alwaysOn) {
      display_toggle(true);
    }
    lastRxTxTime = millis();
  }
  if (espClient.connected()) {
    aprsisState = "OK"; 
  } else {
    aprsisState = "--";
    if (!Config.display.alwaysOn) {
      display_toggle(true);
    }
    lastRxTxTime = millis();
  }*/
  secondLine  = APRS_IS_Utils::checkStatus();// "WiFi: " + wifiState + "/ APRS-IS: " + aprsisState;
  
  show_display(firstLine, secondLine, thirdLine, fourthLine, 0);

  while (espClient.connected()) {
    uint32_t lastRxTx = millis() - lastRxTxTime;
    if (!Config.display.alwaysOn) {
      if (lastRxTx >= Config.display.timeout*1000) {
        display_toggle(false);
      }
    }
    thirdLine = "";
    fourthLine = "";

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
      String aprsisData, aprsisPacket, newLoraPacket, Sender, AddresseeAndMessage, Addressee, receivedMessage;
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
              String ackPacket = Config.callsign + ">APLR10,TCPIP,qAC::" + Sender + ":" + ackMessage + "\n";
              espClient.write(ackPacket.c_str());
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1, AddresseeAndMessage.indexOf("{"));
            } else {
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
            }
            if (receivedMessage.indexOf("?") == 0) {
              Serial.println("Received Query APRS-IS : " + aprsisPacket);
              String queryAnswer = processQueryAnswer(receivedMessage, Sender, "APRSIS");
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
            newLoraPacket = processAPRSISPacket(aprsisPacket);
            deleteNotHeardStation();
            validHeardStation = checkValidHeardStation(Addressee);
            if (validHeardStation) {
              LoRaUtils::sendNewPacket("APRS", newLoraPacket);
              display_toggle(true);
              lastRxTxTime = millis();
              receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
              show_display(firstLine, secondLine, Sender + " -> " + Addressee, receivedMessage, 1000);
            }
          }
        }        
      }
    }
    if (statusAfterBoot) {
      utils::processStatus();
    }
  }
}