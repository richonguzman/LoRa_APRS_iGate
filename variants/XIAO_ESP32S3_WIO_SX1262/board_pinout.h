#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define RADIO_SCLK_PIN      7
    #define RADIO_MISO_PIN      8
    #define RADIO_MOSI_PIN      9
    #define RADIO_CS_PIN        41
    #define RADIO_RST_PIN       42
    #define RADIO_DIO1_PIN      39
    #define RADIO_BUSY_PIN      40

    #define RADIO_HAS_RF_SWITCH //  DIO02
    #define RADIO_RF_SWITCH     38

    #define BUTTON_PIN          21
    #define INTERNAL_LED_PIN    48
    
#endif