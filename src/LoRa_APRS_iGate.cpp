#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <vector>

#include "pins_config.h"
#include "igate_config.h"
#include "display.h"

#define VERSION   "2023.05.23"

WiFiClient      espClient;
String          ConfigurationFilePath = "/igate_conf.json";
Configuration   Config(ConfigurationFilePath);

uint32_t        lastTxTime            = 0;
static bool     beacon_update         = true;
unsigned long   previousWiFiMillis    = 0;
static uint32_t lastRxTxTime          = millis();

static int      myWiFiAPIndex         = 0;
int             myWiFiAPSize          = Config.wifiAPs.size();
WiFi_AP         *currentWiFi          = &Config.wifiAPs[myWiFiAPIndex];

std::vector<String> lastHeardStation;
std::vector<String> lastHeardStation2;
static uint32_t startUpTime           = millis();

String firstLine, secondLine, thirdLine, fourthLine, iGateLatitude, iGateLongitude;

void setup_wifi() {
  int status = WL_IDLE_STATUS;
  Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.print("' ");
  show_display(" ", "Connecting to Wifi:", currentWiFi->ssid + " ...", 0);
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
      show_display(" ", "Connect to Wifi:", currentWiFi->ssid + " ...", 0);
      WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    }
  }
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
}

void setup_lora() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ); 
  if (!LoRa.begin(Config.loramodule.frequency)) {
    Serial.println("Starting LoRa failed!");
    while (true) {
    }
  }
  LoRa.setSpreadingFactor(Config.loramodule.spreading_factor);
  LoRa.setSignalBandwidth(Config.loramodule.signal_bandwidth);
  LoRa.setCodingRate4(Config.loramodule.coding_rate4);
  LoRa.enableCrc();
  LoRa.setTxPower(Config.loramodule.power);
  Serial.println("LoRa init done!\n");
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
    aprsauth = "user " + Config.callsign + " pass " + Config.aprs_is.passcode + " vers " + Config.aprs_is.software_name + " " + Config.aprs_is.software_version + " filter t/m/" + Config.callsign + "/" + (String)Config.aprs_is.reporting_distance + "\n\r"; 
    espClient.write(aprsauth.c_str());  
    delay(200);
  }
}

String createAPRSPacket(String unprocessedPacket) {
  String callsign_and_path_tracker, payload_tracker, processedPacket;
  int dotsPosition = unprocessedPacket.indexOf(':');
  callsign_and_path_tracker = unprocessedPacket.substring(3, dotsPosition);
  payload_tracker = unprocessedPacket.substring(dotsPosition);
  processedPacket = callsign_and_path_tracker + ",qAO," + Config.callsign + payload_tracker + "\n";
  return processedPacket;
}

bool checkValidHeardStation(String station) {
  for (int i=0; i<lastHeardStation.size(); i++) {
    if (lastHeardStation[i].substring(0,lastHeardStation[i].indexOf(",")) == station) {
      return true;
    } 
  }
  Serial.println("   ---> Station not Heard for last 30 min (Not Tx)\n");
  return false;
}

void deleteNotHeardStation() {
  uint32_t minReportingTime = 30*60*1000; // 30 minutes // from .json and CONFIGURATION?????
  for (int i=0; i<lastHeardStation.size(); i++) {
    String deltaTimeString = lastHeardStation[i].substring(lastHeardStation[i].indexOf(",")+1);
    uint32_t deltaTime = deltaTimeString.toInt();
    if ((millis() - deltaTime) < minReportingTime) {
      lastHeardStation2.push_back(lastHeardStation[i]);
    }
  }
  lastHeardStation.clear();
  for (int j=0; j<lastHeardStation2.size(); j++) {
    lastHeardStation.push_back(lastHeardStation2[j]);
  }
  lastHeardStation2.clear();
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
  Serial.println("Stations Near (last 30 minutes):");
  for (int k=0; k<lastHeardStation.size(); k++) {
    Serial.println(lastHeardStation[k]);
  }
  Serial.println(" ");
}

void sendNewLoraPacket(String typeOfMessage, String newPacket) {
  LoRa.beginPacket();
  LoRa.write('<');
  if (typeOfMessage == "APRS")  {
    LoRa.write(0xFF);
  } else if (typeOfMessage == "LoRa") {
    LoRa.write(0xF8);
  }
  LoRa.write(0x01);
  LoRa.write((const uint8_t *)newPacket.c_str(), newPacket.length());
  LoRa.endPacket();
  Serial.print("\n---> LoRa Packet Tx : ");
}

String processQueryAnswer(String query, String station) {
  String processedQuery, queryAnswer;
  if (query.indexOf("?APRS?") == 0 || query.indexOf("?aprs?") == 0 || query.indexOf("?Aprs?") == 0) {
    processedQuery = "?APRSV ?APRSP ?APRSL ?APRSH ?WHERE callsign";
  } else if (query.indexOf("?APRSV") == 0 || query.indexOf("?aprsv") == 0 || query.indexOf("?Aprsv") == 0) {
    processedQuery = Config.aprs_is.software_name + " " + Config.aprs_is.software_version;
  } else if (query.indexOf("?APRSP") == 0 || query.indexOf("?aprsp") == 0 || query.indexOf("?Aprsp") == 0) {
    processedQuery = "iGate QTH: " + String(currentWiFi->latitude) + " " + String(currentWiFi->longitude);
  } else if (query.indexOf("?APRSL") == 0 || query.indexOf("?aprsl") == 0 || query.indexOf("?Aprsl") == 0) {
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
  queryAnswer = Config.callsign + ">APLG01,RFONLY::" + station + ":" + processedQuery;
  return queryAnswer;
}

void checkReceivedPacket(String packet) {
  bool queryMessage = false;
  String aprsPacket, Sender, AddresseeAndMessage, Addressee, ackMessage, receivedMessage, queryAnswer;
  Serial.print("Received Lora Message : " + String(packet));
  if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {
    Serial.print("   ---> Valid LoRa Packet!");
    Sender = packet.substring(3,packet.indexOf(">"));
    if (Sender != Config.callsign) {   // avoid listening yourself by digirepeating                                
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
            //sendMessage(Sender, ackMessage);
            receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1, AddresseeAndMessage.indexOf("{"));
          } else {
            receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":")+1);
          }
          if (receivedMessage.indexOf("?") == 0) {
            queryMessage = true;
            queryAnswer = processQueryAnswer(receivedMessage, Sender);
            delay(2000);
            if (!Config.display.always_on) {
              display_toggle(true);
            }
            lastRxTxTime = millis();
            sendNewLoraPacket("APRS", queryAnswer); 
            show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + Sender, "Type --> QUERY",  1000);
            Serial.println(queryAnswer);  
          } 
        }
      }
      if (!queryMessage) {
        aprsPacket = createAPRSPacket(packet);
        if (!Config.display.always_on) {
          display_toggle(true);
        }
        lastRxTxTime = millis();
        espClient.write(aprsPacket.c_str());
        Serial.println("   ---> Packet Uploaded to APRS-IS");
        deleteNotHeardStation();
        updateLastHeardStation(Sender);
        if (aprsPacket.indexOf("::") >= 10) {
          show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + Sender, "Type --> MESSAGE",  1000);
        } else if (aprsPacket.indexOf(":>") >= 10) {
          show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + Sender, "Type --> NEW STATUS", 1000);
        } else {
          show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + Sender, "Type --> GPS BEACON", 1000);
        }
      }
    }    
  } else {
    Serial.println("   ---> Not Valid LoRa Packet (Ignore)\n");
  }
}

String process_aprsisPacket(String aprsisMessage) {
  String firstPart, messagePart, newLoraPacket;
  aprsisMessage.trim();
  firstPart = aprsisMessage.substring(0, aprsisMessage.indexOf(","));
  messagePart = aprsisMessage.substring(aprsisMessage.indexOf("::")+2);
  newLoraPacket = firstPart + ",TCPIP," + Config.callsign + "::" + messagePart;
  Serial.println("Received from APRS-IS : " + aprsisMessage);
  Serial.print("Reformated Packet     : " + newLoraPacket);
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
  setup_lora();
  iGateLatitude = create_lat_aprs(currentWiFi->latitude);
  iGateLongitude = create_lng_aprs(currentWiFi->longitude);
}

void loop() {
  String wifiState, aprsisState;
  firstLine = "LoRa iGate: " + Config.callsign;
  secondLine = " ";
  thirdLine   = " ";
  fourthLine  = " ";
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
    if (!Config.display.always_on) {
      display_toggle(true);
    }
    lastRxTxTime = millis();
  }
  if (espClient.connected()) {
    aprsisState = "OK"; 
  } else {
    aprsisState = "--";
    if (!Config.display.always_on) {
      display_toggle(true);
    }
    lastRxTxTime = millis();
  }
  secondLine  = "WiFi: " + wifiState + "/ APRS-IS: " + aprsisState;
  
  show_display(firstLine, secondLine, thirdLine, fourthLine, 0);

  while (espClient.connected()) {
    uint32_t lastRxTx = millis() - lastRxTxTime;
    if (!Config.display.always_on) {
      if (lastRxTx >= Config.display.timeout*1000) {
        display_toggle(false);
      }
    }
    thirdLine = " ";
    fourthLine = " ";

    show_display(firstLine, secondLine, thirdLine, fourthLine, 0);
    uint32_t lastTx = millis() - lastTxTime;
    if (lastTx >= Config.beacon_interval*60*1000) {
      beacon_update = true;    
    }
    if (beacon_update) {
      display_toggle(true);
      Serial.println("---- Sending iGate Beacon ----");
      
      String iGateBeaconPacket = Config.callsign + ">APLG01,qAC:=" + iGateLatitude + "L" + iGateLongitude + "&" + Config.comment + "\n";
      //Serial.println(iGateBeaconPacket);
      espClient.write(iGateBeaconPacket.c_str()); 
      lastTxTime = millis();
      lastRxTxTime = millis();
      show_display(firstLine, secondLine, thirdLine, ">SENDING iGate BEACON", 1000);
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
      String aprsisData, aprsisPacket, newLoraMessage, Sender, AddresseAndMessage, Addressee, Message;
      bool validHeardStation = false;
      aprsisData = espClient.readStringUntil('\r'); // or '\n'
      aprsisPacket.concat(aprsisData);
      if (!aprsisPacket.startsWith("#")){
        if (aprsisPacket.indexOf("::")>0) {
          newLoraMessage = process_aprsisPacket(aprsisPacket);
          Sender = newLoraMessage.substring(0,newLoraMessage.indexOf(">"));
          AddresseAndMessage = newLoraMessage.substring(newLoraMessage.indexOf("::")+2);
          Addressee = AddresseAndMessage.substring(0, AddresseAndMessage.indexOf(":"));
          Addressee.trim();
          Message = AddresseAndMessage.substring(AddresseAndMessage.indexOf(":")+1);
          deleteNotHeardStation();
          validHeardStation = checkValidHeardStation(Addressee);
          if (validHeardStation) {
            sendNewLoraPacket("APRS", newLoraMessage);
            display_toggle(true);
            lastRxTxTime = millis();
            show_display(firstLine, secondLine, Sender + " -> " + Addressee, Message, 2000);
          }   
        }        
      }
    }
  }
}