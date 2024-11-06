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
    #define HAS_EPAPER
    #define EPAPER_BUSY             7
    #define EPAPER_RST              6
    #define EPAPER_DC               5
    #define EPAPER_CS               4
    #define EPAPER_SCL              3
    #define EPAPER_SDA              2

    //  Aditional Config
    #define INTERNAL_LED_PIN        18
    #define BATTERY_PIN             20
    #define ADC_CTRL                19
    #define VEXT_CTRL               45
    #define BOARD_I2C_SDA           37
    #define BOARD_I2C_SCL           36

#endif