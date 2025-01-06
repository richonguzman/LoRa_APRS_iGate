#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1278
    #define RADIO_SCLK_PIN      5
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      27
    #define RADIO_CS_PIN        18
    #define RADIO_RST_PIN       23
    #define RADIO_BUSY_PIN      26

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA            21
    #define OLED_SCL            22
    #define OLED_RST            -1

    //  GPS
    #define HAS_GPS
    #define GPS_RX              12
    #define GPS_TX              34

    //  OTHER
    #define INTERNAL_LED_PIN    2
    #define BATTERY_PIN         35

#endif