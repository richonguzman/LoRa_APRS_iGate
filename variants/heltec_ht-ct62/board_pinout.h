#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define RADIO_SCLK_PIN          10
    #define RADIO_MISO_PIN          6
    #define RADIO_MOSI_PIN          7
    #define RADIO_CS_PIN            8
    #define RADIO_RST_PIN           5
    #define RADIO_DIO1_PIN          3
    #define RADIO_BUSY_PIN          4
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_NUM_3

    //  Aditional Config
    #define BATTERY_PIN             1

#endif