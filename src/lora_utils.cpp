#include <LoRa.h>
#include "igate_config.h"
#include "display.h"

extern Configuration Config;

namespace LoRaUtils {

void setup() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  if (!LoRa.begin(Config.loramodule.frequency)) {
    Serial.println("Starting LoRa failed!");
    show_display("ERROR", "Starting LoRa failed!");
    while (true) {
      delay(1000);
    }
  }
  LoRa.setSpreadingFactor(Config.loramodule.spreadingFactor);
  LoRa.setSignalBandwidth(Config.loramodule.signalBandwidth);
  LoRa.setCodingRate4(Config.loramodule.codingRate4);
  LoRa.enableCrc();
  LoRa.setTxPower(Config.loramodule.power);
  Serial.println("LoRa init done!\n");  
}

void sendNewPacket(const String &typeOfMessage, const String &newPacket) {
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
  Serial.print("\n---> LoRa Packet Tx    : ");
  Serial.println(newPacket);
}


/*String receivePacket() {
  String loraPacket;
  int packetSize = LoRa.parsePacket();  // Listening for LoRa Packets
  if (packetSize) {
    while (LoRa.available()) {
      int inChar = LoRa.read();
      loraPacket += (char)inChar;
    }
  }

  return loraPacket;
}*/



}