#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

// 
// https://www.kincony.com/download/KC868-A6-schematic.pdf
//
// Adds support for the Kincony KC868-A6 with LoRa SX1278 daughter board. If no SX1278 board, init will halt durring boot. 
// nRF24L01 support out of scope. It is not supported when LoRa daughter board installed
// GPS support 
// OLED screen support
// Todo: Add support for relays and port input/outputs

    //  LoRa Radio 
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          18
    #define RADIO_MISO_PIN          19
    #define RADIO_MOSI_PIN          23
    #define RADIO_CS_PIN             5
    #define RADIO_RST_PIN           21
    #define RADIO_BUSY_PIN          -1
    #define RADIO_DIO0_PIN           2
    
    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                 4
    #define OLED_SCL                15
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)

    //  GPS
    #define HAS_GPS
    #define GPS_BAUDRATE          4800  //Set to match your GPS board
    #define GPS_RX                  -1  //
    #define GPS_TX                  13  //Data TX from GPS module. IO13 is on the breakout connector. IO12 is connected to GNS with a pullup

    // PCF8574
    // https://mischianti.org/category/my-libraries/pcf8574/
    // #define HAS_PCF8574
    // #define PCF8574_ADDR          0x24 //
    // #define PCF8574_SDA              4 // I2C SDA
    // #define PCF8574_SCL             15 // I2C SCL - could realy re-use use OLED_xxxx definitions, but defining for clarity

    // RS485

    // RS232
#endif
