#include <Arduino.h>
#include <LoRa.h>
#include <WiFi.h>
#include <vector>

#include "pins_config.h"
#include "igate_config.h"
#include "display.h"
#include "lora_utils.h"
#include "utils.h"

#define VERSION   "2023.06.04"

WiFiClient      espClient;
String          ConfigurationFilePath = "/igate_conf.json";
Configuration   Config(ConfigurationFilePath);

uint32_t        lastTxTime          = 0;
static bool     beacon_update       = true;
unsigned long   previousWiFiMillis  = 0;
static uint32_t lastRxTxTime        = millis();

static int      myWiFiAPIndex       = 0;
int             myWiFiAPSize        = Config.wifiAPs.size();
WiFi_AP         *currentWiFi        = &Config.wifiAPs[myWiFiAPIndex];
bool            statusAfterBoot     = Config.statusAfterBoot;

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation_temp;

String firstLine, secondLine, thirdLine, fourthLine, iGateLatitude, iGateLongitude;

void setup_wifi() {
  int status = WL_IDLE_STATUS;
  Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.print("' ");
  show_display("", "Connecting to Wifi:", currentWiFi->ssid + " ...", 0);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  unsigned long start = millis();
  WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
    if ((millis() - start) > 15000){
      if(myWiFiAPIndex >= (myWiFiAPSize-1)) {
        myWiFiAPIndex = 0;
      } else {
        myWiFiAPIndex++;
      }
      currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
      start = millis();
      Serial.print("\nConnect to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
      show_display("", "Connect to Wifi:", currentWiFi->ssid + " ...", 0);
      WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    }
  }
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
}

void APRS_IS_connect(){
  int count = 0;
  String aprsauth;
  Serial.println("Connecting to APRS-IS ...");
  while (!espClient.connect(Config.aprs_is.server.c_str(), Config.aprs_is.port) && count < 20) {
    Serial.println("Didn't connect with server...");
    delay(1000);
    espClient.stop();
    espClient.flush();
    Serial.println("Run client.stop");
    Serial.println("Trying to connect with Server: " + String(Config.aprs_is.server) + " AprsServerPort: " + String(Config.aprs_is.port));
    count++;
    Serial.println("Try: " + String(count));
  }
  if (count == 20) {
    Serial.println("Tried: " + String(count) + " FAILED!");
  } else {
    Serial.println("Connected with Server: '" + String(Config.aprs_is.server) + "' (port: " + String(Config.aprs_is.port)+ ")");
    aprsauth = "user " + Config.callsign + " pass " + Config.aprs_is.passcode + " vers " + Config.aprs_is.softwareName + " " + Config.aprs_is.softwareVersion + " filter t/m/" + Config.callsign + "/" + (String)Config.aprs_is.reportingDistance + "\n\r"; 
    espClient.write(aprsauth.c_str());  
    delay(200);
  }
}

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

String double2string(double n, int ndec) {
    String r = "";
    int v = n;
    r += v;
    r += '.';
    int i;
    for (i=0;i<ndec;i++) {
        n -= v;
        n = 10 * abs(n);
        v = n;
        r += v;
    }
    return r;
}

String create_lat_aprs(double lat) {
  String degrees = double2string(lat,6);
  String north_south, latitude, convDeg3;
  float convDeg, convDeg2;

  if (abs(degrees.toFloat()) < 10) {
    latitude += "0";
  }
  Serial.println(latitude);
  if (degrees.indexOf("-") == 0) {
    north_south = "S";
    latitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    north_south = "N";
    latitude += degrees.substring(0,degrees.indexOf("."));
  }
  convDeg = abs(degrees.toFloat()) - abs(int(degrees.toFloat()));
  convDeg2 = (convDeg * 60)/100;
  convDeg3 = String(convDeg2,6);
  latitude += convDeg3.substring(convDeg3.indexOf(".")+1,convDeg3.indexOf(".")+3) + "." + convDeg3.substring(convDeg3.indexOf(".")+3,convDeg3.indexOf(".")+5);
  latitude += north_south;
  return latitude;
}

String create_lng_aprs(double lng) {
  String degrees = double2string(lng,6);
  String east_west, longitude, convDeg3;
  float convDeg, convDeg2;
  
  if (abs(degrees.toFloat()) < 100) {
    longitude += "0";
  }
  if (abs(degrees.toFloat()) < 10) {
    longitude += "0";
  }
  if (degrees.indexOf("-") == 0) {
    east_west = "W";
    longitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    east_west = "E";
    longitude += degrees.substring(0,degrees.indexOf("."));
  }
  convDeg = abs(degrees.toFloat()) - abs(int(degrees.toFloat()));
  convDeg2 = (convDeg * 60)/100;
  convDeg3 = String(convDeg2,6);
  longitude += convDeg3.substring(convDeg3.indexOf(".")+1,convDeg3.indexOf(".")+3) + "." + convDeg3.substring(convDeg3.indexOf(".")+3,convDeg3.indexOf(".")+5);
  longitude += east_west;
  return longitude;
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\nStarting iGate: " + Config.callsign + "   Version: " + String(VERSION));
  setup_display();
  setup_wifi();
  btStop();
  LoRaUtils::setup();
  iGateLatitude = create_lat_aprs(currentWiFi->latitude);
  iGateLongitude = create_lng_aprs(currentWiFi->longitude);
}

void loop() {
  String wifiState, aprsisState;
  firstLine = "LoRa iGate: " + Config.callsign;
  secondLine = "";
  thirdLine   = "";
  fourthLine  = "";
  unsigned long currentWiFiMillis   = millis();

  if ((WiFi.status() != WL_CONNECTED) && (currentWiFiMillis - previousWiFiMillis >= 30000)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousWiFiMillis = currentWiFiMillis;
  }
  
  if (!espClient.connected()) {
    APRS_IS_connect();
  }

  if (WiFi.status() == WL_CONNECTED) {
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
  }
  secondLine  = "WiFi: " + wifiState + "/ APRS-IS: " + aprsisState;
  
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
      String iGateBeaconPacket = Config.callsign + ">APLR10,";
      if (Config.loramodule.enableTx) {
        iGateBeaconPacket += "qAC:=" + iGateLatitude + "L" + iGateLongitude + "a" + Config.comment;
      } else {
        iGateBeaconPacket += "qAC:=" + iGateLatitude + "L" + iGateLongitude + "&" + Config.comment;
      }
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