#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define RADIO_SCLK_PIN          9
    #define RADIO_MISO_PIN          11
    #define RADIO_MOSI_PIN          10
    #define RADIO_CS_PIN            8
    #define RADIO_RST_PIN           12
    #define RADIO_DIO1_PIN          14
    #define RADIO_BUSY_PIN          13

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                17
    #define OLED_SCL                18
    #define OLED_RST                21
    #define OLED_DISPLAY_HAS_RST_PIN

    //  Aditional Config
    #define INTERNAL_LED_PIN        35
    #define BATTERY_PIN             1
    #define VEXT_CTRL               36
    #define ADC_CTRL                37
    #define BOARD_I2C_SDA           41
    #define BOARD_I2C_SCL           42

#endif