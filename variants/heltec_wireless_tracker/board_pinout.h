#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    //  LoRa Radio
    #define HAS_SX1262
    #define RADIO_SCLK_PIN          9       // SX1262 SCK
    #define RADIO_MISO_PIN          11      // SX1262 MISO
    #define RADIO_MOSI_PIN          10      // SX1262 MOSI
    #define RADIO_CS_PIN            8       // SX1262 NSS
    #define RADIO_RST_PIN           12      // SX1262 RST
    #define RADIO_DIO1_PIN          14      // SX1262 DIO1
    #define RADIO_BUSY_PIN          13      // SX1262 BUSY
    
    //  Display
    #define HAS_DISPLAY
    #define HAS_TFT

    //  Aditional Config
    #define INTERNAL_LED_PIN        18
    #define BATTERY_PIN             1    
    #define ADC_CTRL                2   // HELTEC Wireless Tracker ADC_CTRL = HIGH powers the voltage divider to read BatteryPin. Only on V05 = V1.1
    #define VEXT_CTRL               3   // To turn on GPS and TFT
    #define BOARD_I2C_SDA           7
    #define BOARD_I2C_SCL           6

    //  GPS
    #define HAS_GPS
    #define GPS_BAUDRATE            115200
    #define GPS_RX                  34
    #define GPS_TX                  33

#endif