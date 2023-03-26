#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "pins_config.h"
#include "igate_config.h"
#include "display.h"
#include "iGate_config_OLD.h"

WiFiClient      espClient;
String          ConfigurationFilePath = "/igate_conf.json";
Configuration   Config(ConfigurationFilePath);


uint32_t        lastTxTime            = 0;
static bool     beacon_update         = true;
unsigned long   previousWiFiMillis    = 0;
static uint32_t lastRxTxTime          = millis();
//static bool     displayEcoMode        = true;

static int      myWiFiAPIndex         = 1;
int             myWiFiAPSize          = Config.wifiAPs.size();
WiFi_AP         *currentWiFi          = &Config.wifiAPs[myWiFiAPIndex];

String firstLine, secondLine, thirdLine, fourthLine;

void setup_wifi() {
  int status = WL_IDLE_STATUS;
  Serial.print("\nConnecting to '"); Serial.print(currentWiFi->ssid); Serial.println("' WiFi ...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
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
  int two_dots_position = unprocessedPacket.indexOf(':');
  callsign_and_path_tracker = unprocessedPacket.substring(3, two_dots_position);
  payload_tracker = unprocessedPacket.substring(two_dots_position);
  processedPacket = callsign_and_path_tracker + ",qAO," + Config.callsign + payload_tracker + "\n";
  return processedPacket;
}

void validate_and_upload(String packet) {
  String aprsPacket;
  if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.substring(4, 5) != "}")) {
    Serial.println("   ---> Valid LoRa Packet!");
    aprsPacket = createAPRSPacket(packet);
    display_toggle(true);
    lastRxTxTime = millis();
    espClient.write(aprsPacket.c_str());
    Serial.print("Message uploaded      : "); Serial.println(aprsPacket);
    if (aprsPacket.indexOf("::") >= 10) {
      show_display("LoRa iGate: " + Config.callsign, secondLine, String(aprsPacket.substring(0,aprsPacket.indexOf(">"))) + " MESSAGE",  1000);
    } else if (aprsPacket.indexOf(":>") >= 10) {
      show_display("LoRa iGate: " + Config.callsign, secondLine, String(aprsPacket.substring(0,aprsPacket.indexOf(">"))) + " NEW STATUS", 1000);
    } else {
      show_display("LoRa iGate: " + Config.callsign, secondLine, String(aprsPacket.substring(0,aprsPacket.indexOf(">"))) + " GPS BEACON", 1000);
    }
    
  } else {
    Serial.println("   ---> Not Valid LoRa Packet (Ignore)");
  }
}

String process_aprsisPacket(String aprsisMessage) {
  String firstPart, messagePart, newLoraPacket;
  firstPart = aprsisMessage.substring(0, aprsisMessage.indexOf("*"));
  messagePart = aprsisMessage.substring(aprsisMessage.indexOf("::")+2);
  newLoraPacket = "}" + firstPart + "," + Config.callsign + "*::" + messagePart + "\n";
  Serial.print(newLoraPacket);
  return newLoraPacket;
}

/*void showConfig() {
  Serial.println(myWiFiAPSize);
  Serial.println(myWiFiAPIndex);
  if(myWiFiAPIndex >= (myWiFiAPSize-1)) {
    myWiFiAPIndex = 0;
  } else {
    myWiFiAPIndex++;
  }
  Serial.println(myWiFiAPIndex);
  currentWiFi = &Config.wifiAPs[myWiFiAPIndex];
  Serial.println(currentWiFi->ssid);
  Serial.println(currentWiFi->password);
  Serial.println(currentWiFi->latitude);
  Serial.println(currentWiFi->longitude);
  Serial.println(Config.callsign);
  Serial.println(Config.comment);
  Serial.println(Config.aprs_is.active);
  Serial.println(Config.aprs_is.passcode);
  Serial.println(Config.aprs_is.server);
  Serial.println(Config.aprs_is.port);
  Serial.println(Config.loramodule.frequency);
  Serial.println(Config.loramodule.spreading_factor);
  Serial.println(Config.loramodule.signal_bandwidth);
  Serial.println(Config.loramodule.coding_rate4);
  Serial.println(Config.loramodule.power);
  Serial.println(Config.display.always_on);
  Serial.println(Config.display.timeout);
  Serial.println(" ");
}*/

void setup() {
  Serial.begin(115200);
  Serial.println("Starting iGate: " + Config.callsign + "\n");
  setup_display();
  setup_wifi();
  btStop();
  setup_lora();
}

void loop() {
  //showConfig();
  //delay(2000);
  String wifiState, aprsisState;
  firstLine = "LoRa iGate: " + Config.callsign;
  secondLine = " ";
  thirdLine   = " ";
  fourthLine  = " ";
  unsigned long currentWiFiMillis   = millis();

  if ((WiFi.status() != WL_CONNECTED) && (currentWiFiMillis - previousWiFiMillis >= WifiCheckInterval)) {   // if WiFi is down, try reconnecting
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousWiFiMillis = currentWiFiMillis;
  }
  if (!espClient.connected()) {
    APRS_IS_connect();
  }
  if (WiFi.status() == WL_CONNECTED) wifiState = "OK"; else wifiState = "--"; display_toggle(true); lastRxTxTime = millis();
  if (espClient.connected()) aprsisState = "OK"; else aprsisState = "--"; display_toggle(true); lastRxTxTime = millis();

  secondLine  = "WiFi: " + wifiState + "/ APRS-IS: " + aprsisState;
  
  show_display(firstLine, secondLine, thirdLine, fourthLine, 0);

  while (espClient.connected()) {
    uint32_t lastRxTx = millis() - lastRxTxTime;
    if (Config.display.always_on) {
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
      //String iGateBeaconPacket = Config.callsign + ">APRS,TCPIP*,qAC,CHILE:=" + currentWiFi->latitude + "L" + currentWiFi->longitude + "&" + Config.comment;
      String iGateBeaconPacket = Config.callsign + ">APRS,TCPIP*,qAC,CHILE:=" + Latitude + "L" + Longitude + "&" + Config.comment + "\n";
      //Serial.println(iGateBeaconPacket);
      espClient.write(iGateBeaconPacket.c_str()); 
      lastTxTime = millis();
      display_toggle(true);
      lastRxTxTime = millis();
      show_display(firstLine, secondLine, thirdLine, "*SENDING iGate BEACON", 1000);
      beacon_update = false;
    }

    String loraPacket = "";
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      while (LoRa.available()) {
        int inChar = LoRa.read();
        loraPacket += (char)inChar;
      }
      Serial.print("\nReceived Lora Message : " + String(loraPacket));
      validate_and_upload(loraPacket);
    }
    
    if (espClient.available()) {
      String aprsisData, aprsisPacket, newLoraMessage, Sender, AddresseAndMessage, Addressee, Message;
      aprsisData = espClient.readStringUntil('\n');
      aprsisPacket.concat(aprsisData);
      if (!aprsisPacket.startsWith("#")){
        if (aprsisPacket.indexOf("::")>0) {
          Serial.println("APRS-IS to Tracker    : " + aprsisPacket);
          newLoraMessage = process_aprsisPacket(aprsisPacket);
          LoRa.beginPacket(); 
          LoRa.write('<');
          LoRa.write(0xFF);
          LoRa.write(0x01);
          LoRa.write((const uint8_t *)newLoraMessage.c_str(), newLoraMessage.length());
          LoRa.endPacket();
          Serial.println("packet LoRa enviado!");
          display_toggle(true);
          lastRxTxTime = millis();
          Sender = newLoraMessage.substring(1,newLoraMessage.indexOf(">"));
          AddresseAndMessage = newLoraMessage.substring(newLoraMessage.indexOf("::")+2);
          Addressee = AddresseAndMessage.substring(0, AddresseAndMessage.indexOf(":"));
          Message = AddresseAndMessage.substring(AddresseAndMessage.indexOf(":")+1);
          show_display(firstLine, secondLine, Sender + " --> " + Addressee, Message, 2000);
        }        
      }
    }
  }
}