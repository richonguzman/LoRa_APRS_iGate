#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          5
    #define RADIO_MISO_PIN          19
    #define RADIO_MOSI_PIN          27
    #define RADIO_CS_PIN            18
    #define RADIO_RST_PIN           14
    #define RADIO_BUSY_PIN          26

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                4
    #define OLED_SCL                15
    #define OLED_RST                16
    #define OLED_DISPLAY_HAS_RST_PIN

    //  Aditional Config
    #define INTERNAL_LED_PIN        25
    #define BATTERY_PIN             37
    #define ADC_CTRL                21
    
#endif