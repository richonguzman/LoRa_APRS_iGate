#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define RADIO_SCLK_PIN      14
    #define RADIO_MOSI_PIN      2
    #define RADIO_MISO_PIN      15
    #define RADIO_CS_PIN        5
    #define RADIO_BUSY_PIN      36
    #define RADIO_RST_PIN       4
    #define RADIO_WAKEUP_PIN    RADIO_BUSY_PIN
    #define GPIO_WAKEUP_PIN     GPIO_NUM_36

    //LAN7810A-EZC Ethernet, RMII LAN8720 (Olimex, etc.)
    #define HAS_LAN
    #undef  ETH_PHY_TYPE
    #undef  ETH_PHY_ADDR
    #define ETH_PHY_TYPE        ETH_PHY_LAN8720
    #define ETH_PHY_ADDR         0
    #define ETH_PHY_MDC         23
    #define ETH_PHY_MDIO        18
    #define ETH_PHY_POWER       12
    #define ETH_CLK_MODE        ETH_CLOCK_GPIO17_OUT

    //  Display
    #define HAS_DISPLAY

    #define OLED_SDA    13
    #define OLED_SCL    16
    #define OLED_RST    -1

    //  Aditional Config
    #define BATTERY_PIN         35

#endif