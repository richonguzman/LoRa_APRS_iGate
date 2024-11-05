#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    #define HAS_SX1268
    #define HAS_1W_LORA

    /*#undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                18
    #define OLED_SCL                17
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)*/

    #define RADIO_SCLK_PIN          8
    #define RADIO_MISO_PIN          9
    #define RADIO_MOSI_PIN          10
    #define RADIO_CS_PIN            5
    #define RADIO_RST_PIN           4
    #define RADIO_DIO1_PIN          2
    #define RADIO_BUSY_PIN          3
    #define RADIO_RXEN              6
    #define RADIO_TXEN              7

#endif