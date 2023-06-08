#include "aprs_is_utils.h"
#include <WiFi.h>
#include "configuration.h"
#include "display.h"

extern Configuration  Config;
extern WiFiClient     espClient;
extern int            internalLedPin;
extern uint32_t       lastRxTxTime;
extern int            stationMode;

namespace APRS_IS_Utils {

void connect(){
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
    Serial.println("Connected with Server: " + String(Config.aprs_is.server) + " Port: " + String(Config.aprs_is.port));
    aprsauth = "user " + Config.callsign + " pass " + Config.aprs_is.passcode + " vers " + Config.aprs_is.softwareName + " " + Config.aprs_is.softwareVersion + " filter t/m/" + Config.callsign + "/" + (String)Config.aprs_is.reportingDistance + "\n\r"; 
    espClient.write(aprsauth.c_str());
    delay(200);
  }
}

String checkStatus() {
  String wifiState, aprsisState;
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
  return "WiFi: " + wifiState + "/ APRS-IS: " + aprsisState;
}

String createPacket(String packet) {
  if (stationMode > 1) {
    return packet.substring(3, packet.indexOf(':')) + ",qAR," + Config.callsign + packet.substring(packet.indexOf(':')) + "\n";
  } else {
    return packet.substring(3, packet.indexOf(':')) + ",qAO," + Config.callsign + packet.substring(packet.indexOf(':')) + "\n";
  }
}

void processLoRaPacket(String packet) {




  
}

/*void processSplitedMessage(String addressee, String message1, String message2) {
  espClient.write((Config.callsign + ">APRS,qAC::" + addressee + ":" + message1 + "\n").c_str()); 
  Serial.println("-------> " + message1);
  Serial.println("(waiting for second part)");
  delay(5000);
  espClient.write((Config.callsign + ">APRS,qAC::" + addressee + ":" + message2 + "\n").c_str());   
  Serial.println("-------> " + message2);
}*/

}