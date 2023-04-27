#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPIFFS.h>

#include "pins_config.h"
#include "igate_config.h"
#include "display.h"

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

String firstLine, secondLine, thirdLine, fourthLine;

void setup_wifi() {
  int status = WL_IDLE_STATUS;
  Serial.print("\nConnect to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
  show_display(" ", "Connect to Wifi:", currentWiFi->ssid + " ...", 0);
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
    if (!Config.display.always_on) {
      display_toggle(true);
    }
    lastRxTxTime = millis();
    espClient.write(aprsPacket.c_str());
    Serial.print("Message uploaded      : "); Serial.println(aprsPacket);
    if (aprsPacket.indexOf("::") >= 10) {
      show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + String(aprsPacket.substring(0,aprsPacket.indexOf(">"))), "Type --> MESSAGE",  1000);
    } else if (aprsPacket.indexOf(":>") >= 10) {
      show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + String(aprsPacket.substring(0,aprsPacket.indexOf(">"))), "Type --> NEW STATUS", 1000);
    } else {
      show_display("LoRa iGate: " + Config.callsign, secondLine, "Callsign = " + String(aprsPacket.substring(0,aprsPacket.indexOf(">"))), "Type --> GPS BEACON", 1000);
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

  if (degrees.indexOf("-") == 0) {
    north_south = "S";
    latitude = degrees.substring(1,degrees.indexOf("."));
  } else {
    north_south = "N";
    latitude = degrees.substring(0,degrees.indexOf("."));
  }
  if (latitude.length() == 1) {
    latitude = "0" + latitude;
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
  if (degrees.indexOf("-") == 0) {
    east_west = "W";
    longitude += degrees.substring(1,degrees.indexOf("."));
  } else {
    east_west = "E";
    longitude += degrees.substring(0,degrees.indexOf("."));
  }
  if (longitude.length() == 1) {
    longitude = "0" + longitude;
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
  Serial.println("Starting iGate: " + Config.callsign + "\n");
  setup_display();
  setup_wifi();
  btStop();
  setup_lora();
}

void loop() {
  String wifiState, aprsisState, iGateLatitude, iGateLongitude;
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
      iGateLatitude = create_lat_aprs(currentWiFi->latitude);
      iGateLongitude = create_lng_aprs(currentWiFi->longitude);
      String iGateBeaconPacket = Config.callsign + ">APLG01,qAC:=" + iGateLatitude + "L" + iGateLongitude + "&" + Config.comment + "\n";
      //Serial.println(iGateBeaconPacket);
      espClient.write(iGateBeaconPacket.c_str()); 
      lastTxTime = millis();
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
          show_display(firstLine, secondLine, Sender + " -> " + Addressee, Message, 2000);
        }        
      }
    }
  }
}