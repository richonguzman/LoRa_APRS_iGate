#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1276
    #define RADIO_SCLK_PIN          18
    #define RADIO_MISO_PIN          19
    #define RADIO_MOSI_PIN          23
    #define RADIO_CS_PIN            2
    #define RADIO_RST_PIN           0
    #define RADIO_BUSY_PIN          32

    //  Display    
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                21
    #define OLED_SCL                22
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)

    //  Aditional Config
    #define INTERNAL_LED_PIN        13      // 13 for V1.1 and 12 for V1.0
    #define BATTERY_PIN             35

    #define HAS_A7670
    #define A7670_PWR_PIN           4
    #define A7670_ResetPin          5
    #define A7670_TX_PIN            26
    #define A7670_RX_PIN            27

#endif