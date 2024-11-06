#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          15
    #define RADIO_MISO_PIN          13
    #define RADIO_MOSI_PIN          12
    #define RADIO_CS_PIN            14
    #define RADIO_RST_PIN           2
    #define RADIO_BUSY_PIN          25

    //  Display    
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                5
    #define OLED_SCL                4
    #define OLED_RST                -1

#endif