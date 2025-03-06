#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          4
    #define RADIO_MISO_PIN          5
    #define RADIO_MOSI_PIN          6
    #define RADIO_CS_PIN            20
    #define RADIO_RST_PIN           3
    #define RADIO_BUSY_PIN          2

    //  Ethernet    W5500
    #define HAS_ETHERNET
    #define Ethernet_SCK            4   //  W5550   SCK
    #define Ethernet_MISO           5   //  W5550   MI
    #define Ethernet_MOSI           6   //  W5550   MO
    #define Ethernet_CS             7   //  W5550   CS  (CS = NSS = SS)
    //#define Ethernet_VCC      3.3v
    //#define Ethernet_GND      GND

#endif