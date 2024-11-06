#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define RADIO_SCLK_PIN          5
    #define RADIO_MISO_PIN          3
    #define RADIO_MOSI_PIN          6
    #define RADIO_CS_PIN            7
    #define RADIO_RST_PIN           8
    #define RADIO_DIO1_PIN          33
    #define RADIO_BUSY_PIN          34

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                18
    #define OLED_SCL                17
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)

    //  Aditional Config
    #define INTERNAL_LED_PIN        37      // Green Led
    #define BATTERY_PIN             1

#endif