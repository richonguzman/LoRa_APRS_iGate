#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define RADIO_SCLK_PIN          10
    #define RADIO_MISO_PIN           9
    #define RADIO_MOSI_PIN          11
    #define RADIO_CS_PIN            40
    #define RADIO_RST_PIN           46
    #define RADIO_DIO1_PIN          16
    #define RADIO_BUSY_PIN           8
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_16
    #define V_TCXO                  0.0
    #define USE_LDO                 true

    //W5500 Ethernet, SPI using ESP-IDF's driver
    #define HAS_LAN
    #undef  ETH_PHY_TYPE
    #undef  ETH_PHY_ADDR
    #define ETH_PHY_TYPE            ETH_PHY_W5500
    #define ETH_PHY_ADDR            1
    #define ETH_PHY_CS              45
    #define ETH_PHY_IRQ             14
    #define ETH_PHY_RST             13
    #define ETH_PHY_SPI_HOST        SPI2_HOST
    #define ETH_PHY_SPI_SCK         48
    #define ETH_PHY_SPI_MISO        47
    #define ETH_PHY_SPI_MOSI        21

    #define BOARD_SDCARD_CS         12

    //  Display
    #define HAS_DISPLAY  
    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST
    #define OLED_RST                -1

    //  Aditional Config
    #define INTERNAL_LED_PIN        38  
    #define BOARD_I2C_SDA           17
    #define BOARD_I2C_SCL           18

    //  GPS
    #define HAS_GPS
    #define GPS_RX                  42
    #define GPS_TX                  39
    #define GPS_BAUDRATE         38400

#endif