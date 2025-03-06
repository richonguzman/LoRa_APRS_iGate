/*#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "query_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "display.h"
#include "utils.h"*/
#include <Ethernet.h>
#include <SPI.h>
#include "ethernet_utils.h"
#include "board_pinout.h"

/*extern Configuration    Config;
extern uint32_t         lastScreenOn;
extern String           iGateBeaconPacket;
extern String           firstLine;
extern String           secondLine;
extern String           thirdLine;
extern String           fourthLine;
extern String           fifthLine;
extern String           sixthLine;
extern String           seventhLine;
extern bool             backUpDigiMode;*/


byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

namespace ETHERNET_Utils {

    void setup() {
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        pinMode(Ethernet_CS, OUTPUT);
        pinMode(Ethernet_MOSI, OUTPUT);
        pinMode(Ethernet_MISO, INPUT);
        pinMode(Ethernet_SCK, OUTPUT);
        SPI.endTransaction();
        Ethernet.init(Ethernet_CS);
        Ethernet.begin(mac);
        delay(1000);
        Serial.println(Ethernet.localIP());

        delay(1000);
        Serial.println(Ethernet.linkStatus());
        delay(1000);
        Serial.println(Ethernet.linkStatus());
        delay(1000);
        Serial.println(Ethernet.linkStatus());
        delay(1000);
        Serial.println(Ethernet.linkStatus());
        delay(1000);
        if (Ethernet.linkStatus() == LinkON) {
            Serial.println("Ethernet connected...");
        } else {
            Serial.println("Ethernet not connected...");
        }
    }

}