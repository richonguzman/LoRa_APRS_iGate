#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include "iGate_config.h"
#include "pins_config.h"

#define VERSION "V.0.1.0"

WiFiClient espClient;
uint32_t lastTxTime                 = 0;
static bool beacon_update           = true;
unsigned long previousWiFiMillis    = 0;

void setup_wifi() {
  int status = WL_IDLE_STATUS;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnecting to '"); Serial.print(WIFI_SSID); Serial.println("' WiFi ...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("Connected as ");
  Serial.println(WiFi.localIP());
}

void setup_lora() {
  Serial.println("Set LoRa pins!");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ); 

  long freq = 433775000;
  Serial.print("frequency: ");
  Serial.println(String(freq));
  if (!LoRa.begin(freq)) {
    Serial.println("Starting LoRa failed!");
    while (true) {
    }
  }
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125000);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  LoRa.setTxPower(20);
  Serial.println("LoRa init done!\n");
}

void APRS_IS_connect(){
  int count = 0;
  String aprsauth;
  Serial.println("Connecting to APRS-IS ...");
  while (!espClient.connect(AprsServer.c_str(), AprsServerPort) && count < 20) {
    Serial.println("Didn't connect with server...");
    delay(1000);
    espClient.stop();
    espClient.flush();
    Serial.println("Run client.stop");
    Serial.println("Trying to connect with Server: " + String(AprsServer) + " AprsServerPort: " + String(AprsServerPort));
    count++;
    Serial.println("Try: " + String(count));
  }
  if (count == 20) {
    Serial.println("Tried: " + String(count) + " FAILED!");
  } else {
    Serial.println("Connected with Server: " + String(AprsServer) + " Port: " + String(AprsServerPort));
    aprsauth = "user " + iGateCallsign + " pass " + iGatePasscode + " vers " + AprsSoftwareName + " " + AprsSoftwareVersion + " filter " + AprsFilter + "\n\r"; 
    espClient.write(aprsauth.c_str());  
    delay(200);
  }
}

String process_loraPacket(String unprocessedPacket) {
  String callsign_and_path_tracker, payload_tracker, processedPacket;
  int two_dots_position = unprocessedPacket.indexOf(':');
  callsign_and_path_tracker = unprocessedPacket.substring(3, two_dots_position);
  payload_tracker = unprocessedPacket.substring(two_dots_position);
  processedPacket = callsign_and_path_tracker + ",qAO," + iGateCallsign + payload_tracker + "\n";
  Serial.print("Message uploaded      : "); Serial.println(processedPacket);
  return processedPacket;
}

void validate_lora_packet(String packet) {
  String packetStart, aprsPacket;
  packetStart = packet.substring(0, 3);
  if (packetStart == "\x3c\xff\x01") {
    Serial.println("   ---> Valid LoRa Packet!");
    aprsPacket = process_loraPacket(packet);
    espClient.write(aprsPacket.c_str());
  } else {
    Serial.println("   ---> Not Valid LoRa Packet (Ignore)");
  }
}

void process_aprsisPacket(String aprsisMessage) {
  Serial.println(aprsisMessage);
  // aqui toda la modificacion del mensaje antes de responder por lora!!!
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  btStop();
  setup_lora();
  Serial.println("Starting iGate: " + iGateCallsign + "\n");
}

void loop() {  
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

  while (espClient.connected()) {
    //while (espClient.available()) {

    uint32_t lastTx = millis() - lastTxTime;
    if (lastTx >= BeaconInterval) {
      beacon_update = true;    
    }
    if (beacon_update) { 
      Serial.println("---- Sending iGate Beacon ----");
      espClient.write(iGateBeaconPacket.c_str()); 
      lastTxTime = millis();
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
      validate_lora_packet(loraPacket);
    }
    
    
    
    if (espClient.available()) {

      String aprsisData, aprsisPacket, subpacket1, receivedMessage, questioner, answerMessage, ackNumber, ackMessage, currentDate, weatherForecast;

      aprsisData = espClient.readStringUntil('\n');
      aprsisPacket.concat(aprsisData);
      if (!aprsisPacket.startsWith("#")){
        Serial.println("APRS-IS to Tracker    : " + aprsisPacket);
        process_aprsisPacket(aprsisPacket);
      }
    }
        /*
        if (packet.indexOf("WRCLP") > 0){
          if (packet.indexOf("::")>0) {
            subpacket1 = packet.substring(packet.indexOf("::")+2);
            receivedMessage = subpacket1.substring(subpacket1.indexOf(":")+1);
            questioner = packet.substring(0,packet.indexOf(">"));
            //Serial.println(receivedMessage);

            if (receivedMessage.indexOf("{")>0) {                                  // if questioner solicitates ack 
              ackNumber = receivedMessage.substring(receivedMessage.indexOf("{")+1);
              for(int i = questioner.length(); i < 9; i++) {
                questioner += ' ';
              }
              ackMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":ack" + ackNumber + "\n";
              //Serial.print("---> " + ackMessage);
              espClient.write(ackMessage.c_str());
              delay(500);
              receivedMessage = receivedMessage.substring(0,receivedMessage.indexOf("{"));
              //Serial.println(receivedMessage);
            }

            for(int i = questioner.length(); i < 9; i++) {
                questioner += ' ';
            }
            receivedMessage.trim();
            if (receivedMessage == "utc" || receivedMessage == "Utc" || receivedMessage == "UTC" || receivedMessage == "time"|| receivedMessage == "Time" || receivedMessage == "TIME") {
              currentDate = getDateTime();
              answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + currentDate + "\n";
            } else if (receivedMessage == "clima" || receivedMessage == "Clima" || receivedMessage == "CLIMA" || receivedMessage == "weather"|| receivedMessage == "Weather" || receivedMessage == "WEATHER") {
              weatherForecast = GetWeatherForecast(questioner);
              answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + weatherForecast + "\n";  
            } else {
              answerMessage = "WRCLP>APRS,TCPIP*,qAC,CHILE::" + questioner + ":" + "hola " + questioner + "\n";  
            }
            Serial.print("-------> " + answerMessage);
            espClient.write(answerMessage.c_str());          
          }*/

      
    
  }
}
  