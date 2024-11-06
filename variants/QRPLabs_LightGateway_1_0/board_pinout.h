#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1268
    #define RADIO_VCC_PIN           21
    #define RADIO_SCLK_PIN          12
    #define RADIO_MISO_PIN          13
    #define RADIO_MOSI_PIN          11
    #define RADIO_CS_PIN            10
    #define RADIO_RST_PIN           9
    #define RADIO_DIO1_PIN          5
    #define RADIO_BUSY_PIN          6
    #define RADIO_RXEN              42
    #define RADIO_TXEN              14

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                3
    #define OLED_SCL                4
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)

    //  Aditional Config
    #define INTERNAL_LED_PIN        16
    #define BATTERY_PIN             1
    #define BUTTON_PIN              0

#endif