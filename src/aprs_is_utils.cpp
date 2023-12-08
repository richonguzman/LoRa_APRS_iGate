#include <WiFi.h>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration  Config;
extern WiFiClient     espClient;
extern int            internalLedPin;
extern uint32_t       lastScreenOn;
extern int            stationMode;
extern String         firstLine;
extern String         secondLine;
extern String         thirdLine;
extern String         fourthLine;
extern String         fifthLine;
extern String         sixthLine;
extern String         seventhLine;


namespace APRS_IS_Utils {

  void connect(){
    int count = 0;
    String aprsauth;
    Serial.print("Connecting to APRS-IS ...     ");
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
      Serial.println("Connected!\n(Server: " + String(Config.aprs_is.server) + " / Port: " + String(Config.aprs_is.port) +")");
      aprsauth = "user " + Config.callsign + " pass " + Config.aprs_is.passcode + " vers CA2RXU_LoRa_iGate 1.2 filter t/m/" + Config.callsign + "/" + (String)Config.aprs_is.reportingDistance + "\n\r"; 
      espClient.write(aprsauth.c_str());
      delay(200);
    }
  }

  void checkStatus() {
    String wifiState, aprsisState;
    if (WiFi.status() == WL_CONNECTED) {
      wifiState = "OK"; 
    } else {
      wifiState = "--";
      if (!Config.display.alwaysOn) {
        display_toggle(true);
      }
      lastScreenOn = millis();
    }
    if (espClient.connected()) {
      aprsisState = "OK"; 
    } else {
      aprsisState = "--";
      if (!Config.display.alwaysOn) {
        display_toggle(true);
      }
      lastScreenOn = millis();
    }
    secondLine = "WiFi: " + wifiState + "/ APRS-IS: " + aprsisState;
  }

  String createPacket(String packet) {
    if (stationMode>1) {
      return packet.substring(3, packet.indexOf(":`")) + ",qAR," + Config.callsign + packet.substring(packet.indexOf(":`")) + "\n";
    } else {
      return packet.substring(3, packet.indexOf(":`")) + ",qAO," + Config.callsign + packet.substring(packet.indexOf(":`")) + "\n";
    }
  }

  void processLoRaPacket(String packet) {
    bool queryMessage = false;
    String aprsPacket, Sender, AddresseeAndMessage, Addressee, ackMessage, receivedMessage;
    if (packet != "") {
      #ifdef PinPointApp
      Serial.println(packet.substring(3));
      #else
      Serial.print("Received Lora Packet   : " + String(packet));
      #endif    
      if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {
        #ifndef PinPointApp
        Serial.print("   ---> APRS LoRa Packet!");
        #endif
        Sender = packet.substring(3,packet.indexOf(">"));
        if (Sender != Config.callsign) {   // avoid listening yourself by digirepeating
          if (stationMode==2 || stationMode==5) {
            if (packet.indexOf("::") > 10) {    // its a Message!
              AddresseeAndMessage = packet.substring(packet.indexOf("::")+2);  
              Addressee = AddresseeAndMessage.substring(0,AddresseeAndMessage.indexOf(":"));
              Addressee.trim();
              if (Addressee == Config.callsign) {             // its for me!                     
                if (AddresseeAndMessage.indexOf("{")>0) {     // ack?
                  ackMessage = "ack" + AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{")+1);
                  ackMessage.trim();
                  delay(4000);
                  //Serial.println(ackMessage);
                  for(int i = Sender.length(); i < 9; i++) {
                    Sender += ' ';
                  }
                  LoRa_Utils::sendNewPacket("APRS", Config.callsign + ">APLRG1,RFONLY,WIDE1-1::" + Sender + ":" + ackMessage);
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
                  lastScreenOn = millis();
                  show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, "Callsign = " + Sender, "TYPE --> QUERY",  0);
                }
              }
            }
          }
          if (!queryMessage) {
            aprsPacket = createPacket(packet);
            if (!Config.display.alwaysOn) {
              display_toggle(true);
            }
            lastScreenOn = millis();
            espClient.write(aprsPacket.c_str());
            #ifndef PinPointApp
            Serial.println("   ---> Uploaded to APRS-IS");
            #endif
            STATION_Utils::updateLastHeard(Sender);
            Utils::typeOfPacket(aprsPacket, "LoRa-APRS");
            show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
          }
        }    
      } else {
        #ifndef PinPointApp
        Serial.println("   ---> LoRa Packet Ignored (first 3 bytes or TCPIP/NOGATE/RFONLY)\n");
        #endif
      }
    }
  }

  void processAPRSISPacket(String packet) {
    String Sender, AddresseeAndMessage, Addressee, receivedMessage;
    if (!packet.startsWith("#")){
      if (packet.indexOf("::")>0) {
        Sender = packet.substring(0,packet.indexOf(">"));
        AddresseeAndMessage = packet.substring(packet.indexOf("::")+2);
        Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
        Addressee.trim();
        if (Addressee == Config.callsign) {             // its for me!
          if (AddresseeAndMessage.indexOf("{")>0) {     // ack?
            String ackMessage = "ack" + AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{")+1);
            ackMessage.trim();
            delay(4000);
            //Serial.println(ackMessage);
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
            #ifndef PinPointApp
            Serial.println("Received Query APRS-IS : " + packet);
            #endif
            String queryAnswer = QUERY_Utils::process(receivedMessage, Sender, "APRSIS");
            //Serial.println("---> QUERY Answer : " + queryAnswer.substring(0,queryAnswer.indexOf("\n")));
            if (!Config.display.alwaysOn) {
              display_toggle(true);
            }
            lastScreenOn = millis();
            delay(500);
            espClient.write(queryAnswer.c_str());
            fifthLine = "APRS-IS ----> APRS-IS";
            sixthLine = Config.callsign;
            for (int j=sixthLine.length();j<9;j++) {
              sixthLine += " ";
            }
            sixthLine += "> " + Sender;
            seventhLine = "QUERY = " + receivedMessage;
          }
        } else {
          #ifndef PinPointApp
          Serial.print("Received from APRS-IS  : " + packet);
          #endif
          if ((stationMode==2 || stationMode==5) && STATION_Utils::wasHeard(Addressee)) {
            LoRa_Utils::sendNewPacket("APRS", LoRa_Utils::generatePacket(packet));
            display_toggle(true);
            lastScreenOn = millis();
            Utils::typeOfPacket(packet, "APRS-LoRa");
          }
        }
        show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
      }        
    }
  }

  void loop() {
    checkStatus();
    show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);    
    while (espClient.connected()) {
      Utils::checkDisplayInterval();
      Utils::checkBeaconInterval();
      processLoRaPacket(LoRa_Utils::receivePacket());            
      if (espClient.available()) {
        String aprsisPacket;
        aprsisPacket.concat(espClient.readStringUntil('\r'));
        processAPRSISPacket(aprsisPacket);
      }
    }
  }

}