#ifndef PINS_CONFIG_H_
#define PINS_CONFIG_H_

#include <Arduino.h>

#undef OLED_SDA
#undef OLED_SCL
#undef OLED_RST


// LORA MODULES
#if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(ESP32_DIY_LoRa) || defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_LORA32_V2_1_915) || defined(ESP32_DIY_LoRa_915) || defined(TTGO_T_Beam_V1_2_915) || defined(TTGO_T_Beam_V1_0_915)
    #define RADIO_SCLK_PIN      5       // GPIO5    - SX1278 SCK
    #define RADIO_MISO_PIN      19      // GPIO19   - SX1278 MISO
    #define RADIO_MOSI_PIN      27      // GPIO27   - SX1278 MOSI
    #define RADIO_CS_PIN        18      // GPIO18   - SX1278 CS ---> NSS
    #define RADIO_RST_PIN       14      // GPIO14   - SX1278 RST
    #define RADIO_BUSY_PIN      26      // GPIO26   - SX1278 IRQ ---->DIO0
#endif

#if defined(HELTEC_V3) || defined(HELTEC_WSL_V3) || defined(HELTEC_WSL_V3_DISPLAY) || defined(HELTEC_WIRELESS_TRACKER) || defined(HELTEC_WS) || defined(HELTEC_WP)
    #define RADIO_SCLK_PIN      9       // SX1262 SCK
    #define RADIO_MISO_PIN      11      // SX1262 MISO
    #define RADIO_MOSI_PIN      10      // SX1262 MOSI
    #define RADIO_CS_PIN        8       // SX1262 NSS
    #define RADIO_RST_PIN       12      // SX1262 RST
    #define RADIO_DIO1_PIN      14      // SX1262 DIO1
    #define RADIO_BUSY_PIN      13      // SX1262 BUSY
#endif

#if defined(ESP32_DIY_1W_LoRa) || defined(ESP32_DIY_1W_LoRa_915) || defined(ESP32_DIY_1W_LoRa_LLCC68)   // Ebyte E22 400M30S (SX1268) or E22 900M30S (SX1262) or E220 LLCC68
    #define RADIO_SCLK_PIN      18
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      23
    #define RADIO_CS_PIN        5
    #define RADIO_RST_PIN       27
    #define RADIO_DIO1_PIN      12
    #define RADIO_BUSY_PIN      14
    #define RADIO_RXEN          32
    #define RADIO_TXEN          25
#endif

#if defined(ESP32_DIY_1W_LoRa_Mesh_V1_2)  // https://github.com/NanoVHF/Meshtastic-DIY/tree/main/PCB/ESP-32-devkit_EBYTE-E22/Mesh-v1.02-2LCD-FreePins
    #define RADIO_SCLK_PIN      5
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      27
    #define RADIO_CS_PIN        18
    #define RADIO_RST_PIN       23
    #define RADIO_DIO1_PIN      33
    #define RADIO_BUSY_PIN      32
    #define RADIO_RXEN          14
    #define RADIO_TXEN          13
#endif

#ifdef WEMOS_LOLIN32_OLED_DIY_LoRa
    #define RADIO_SCLK_PIN      15
    #define RADIO_MISO_PIN      13
    #define RADIO_MOSI_PIN      12
    #define RADIO_CS_PIN        14
    #define RADIO_RST_PIN       2
    #define RADIO_BUSY_PIN      25
#endif

#if defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
    #define RADIO_SCLK_PIN      5
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      27
    #define RADIO_CS_PIN        18
    #define RADIO_DIO0_PIN      26
    #define RADIO_RST_PIN       23
    #define RADIO_DIO1_PIN      33
    #define RADIO_BUSY_PIN      32
#endif

#if defined(OE5HWN_MeshCom)
    #define RADIO_SCLK_PIN      18
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      23
    #define RADIO_CS_PIN        5
    #define RADIO_RST_PIN       27
    #define RADIO_DIO1_PIN      33
    #define RADIO_BUSY_PIN      26
    #define RADIO_RXEN          14
    #define RADIO_TXEN          13
#endif

#if defined(HELTEC_HTCT62)
    #define RADIO_SCLK_PIN      10   // SX1262 SCK
    #define RADIO_MISO_PIN      6    // SX1262 MISO
    #define RADIO_MOSI_PIN      7    // SX1262 MOSI
    #define RADIO_CS_PIN        8    // SX1262 NSS
    #define RADIO_RST_PIN       5    // SX1262 RST
    #define RADIO_DIO1_PIN      3    // SX1262 DIO1
    #define RADIO_BUSY_PIN      4    // SX1262 BUSY
#endif

#if defined(ESP32_DIY_LoRa_A7670) || defined(ESP32_DIY_LoRa_A7670_915)
    #define RADIO_SCLK_PIN      18
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      23
    #define RADIO_CS_PIN        2
    #define RADIO_RST_PIN       0
    #define RADIO_BUSY_PIN      32
    #define A7670_PWR_PIN       4
    #define A7670_ResetPin      5
    #define A7670_TX_PIN        26
    #define A7670_RX_PIN        27
#endif

#ifdef WEMOS_D1_R32_RA02
    #define RADIO_SCLK_PIN      18
    #define RADIO_MISO_PIN      19
    #define RADIO_MOSI_PIN      23
    #define RADIO_CS_PIN        5
    #define RADIO_BUSY_PIN      12
    #define RADIO_RST_PIN       13
    #define RADIO_DIO1_PIN      14
    #define OLED_SDA            21
    #define OLED_SCL            22
    #define OLED_RST            36
#endif

#if defined(ESP32C3_DIY_1W_LoRa) || defined(ESP32C3_DIY_1W_LoRa_915)
    #define RADIO_SCLK_PIN      8
    #define RADIO_MISO_PIN      9
    #define RADIO_MOSI_PIN      10
    #define RADIO_CS_PIN        5
    #define RADIO_RST_PIN       4
    #define RADIO_DIO1_PIN      2
    #define RADIO_BUSY_PIN      3
    #define RADIO_RXEN          6
    #define RADIO_TXEN          7
#endif

#ifdef WEMOS_S2_MINI_DIY_LoRa
    #define RADIO_SCLK_PIN      36
    #define RADIO_MISO_PIN      37
    #define RADIO_MOSI_PIN      35
    #define RADIO_CS_PIN        34
    #define RADIO_BUSY_PIN      38
    #define RADIO_RST_PIN       33
#endif

#ifdef LIGHTGATEWAY_1_0
    #define RADIO_VCC_PIN       21
    #define RADIO_SCLK_PIN      12
    #define RADIO_MISO_PIN      13
    #define RADIO_MOSI_PIN      11
    #define RADIO_CS_PIN        10
    #define RADIO_RST_PIN       9
    #define RADIO_DIO1_PIN      5
    #define RADIO_BUSY_PIN      6
    #define RADIO_RXEN          42
    #define RADIO_TXEN          14
#endif


// OLED 
#if defined(TTGO_T_LORA32_V2_1) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa) || defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262) || defined(OE5HWN_MeshCom) || defined(ESP32_DIY_LoRa_A7670) || defined(TTGO_T_LORA32_V2_1_915) || defined(ESP32_DIY_LoRa_915) || defined(TTGO_T_Beam_V1_0_915) || defined(TTGO_T_Beam_V1_2_915) || defined(ESP32_DIY_LoRa_A7670_915) || defined(ESP32_DIY_1W_LoRa_915) || defined(ESP32_DIY_1W_LoRa_LLCC68) || defined(ESP32_DIY_1W_LoRa_Mesh_V1_2)
    #define OLED_SDA    21
    #define OLED_SCL    22
    #define OLED_RST    -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

#if defined(HELTEC_V2) || defined(HELTEC_WS)
    #define OLED_SDA    4
    #define OLED_SCL    15
    #define OLED_RST    16
#endif

#if defined(HELTEC_V3)
    #define OLED_SDA    17
    #define OLED_SCL    18
    #define OLED_RST    21
#endif

#ifdef WEMOS_LOLIN32_OLED_DIY_LoRa
    #define OLED_SDA    5
    #define OLED_SCL    4
    #define OLED_RST    -1
#endif

#ifdef LIGHTGATEWAY_1_0
    #define OLED_SDA    3
    #define OLED_SCL    4
    #define OLED_RST    -1
#endif

#if !defined(LIGHTGATEWAY_1_0) && !defined(HELTEC_HTCT62) && !defined(HELTEC_WSL_V3) && !defined(ESP32C3_DIY_1W_LoRa) && !defined(ESP32C3_DIY_1W_LoRa_915) && !defined(WEMOS_S2_MINI_DIY_LoRa)
    #define HAS_DISPLAY
#endif


// Leds and other stuff
#ifdef HELTEC_HTCT62
    #define BATTERY_PIN         1
#endif
#ifdef WEMOS_S2_MINI_DIY_LoRa
    #define INTERNAL_LED_PIN    15
#endif
#if defined(TTGO_T_LORA32_V2_1) || defined(TTGO_T_LORA32_V2_1_915)
    #define INTERNAL_LED_PIN    25      // Green Led
    #define BATTERY_PIN         35
#endif
#if defined(HELTEC_V2)
    #define INTERNAL_LED_PIN    25
    #define BATTERY_PIN         37
    #define ADC_CTRL            21
#endif
#if defined(HELTEC_V3) || defined(HELTEC_WSL_V3) || defined(HELTEC_WSL_V3_DISPLAY) || defined(HELTEC_WS)
    #define INTERNAL_LED_PIN    35
    #define BATTERY_PIN         1
    #define VEXT_CTRL           36
    #define ADC_CTRL            37  
    #define BOARD_I2C_SDA       41
    #define BOARD_I2C_SCL       42
    #ifdef HELTEC_WSL_V3_DISPLAY
        #define OLED_RST    -1
    #endif
#endif

#if defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_LoRa_915) || defined(ESP32_DIY_1W_LoRa) || defined(ESP32_DIY_1W_LoRa_915)
    #define INTERNAL_LED_PIN    2
#endif
#if defined(ESP32_DIY_LoRa_A7670) || defined(ESP32_DIY_LoRa_A7670_915)
    #define INTERNAL_LED_PIN    13      // 13 for V1.1 and 12 for V1.0
    #define BATTERY_PIN         35
#endif

#ifdef HELTEC_WIRELESS_TRACKER
    #define INTERNAL_LED_PIN    18
    #define BATTERY_PIN         1
    #define ADC_CTRL            2   // HELTEC Wireless Tracker ADC_CTRL = HIGH powers the voltage divider to read BatteryPin. Only on V05 = V1.1
    #define VEXT_CTRL           3   // To turn on GPS and TFT
    #define BOARD_I2C_SDA       7
    #define BOARD_I2C_SCL       6
#endif

#ifdef HELTEC_WP
    #define INTERNAL_LED_PIN    18
    #define BATTERY_PIN         20
    #define ADC_CTRL            19
    #define VEXT_CTRL           45
    #define BOARD_I2C_SDA       37
    #define BOARD_I2C_SCL       36
    #define EPAPER_BUSY         7
    #define EPAPER_RST          6
    #define EPAPER_DC           5
    #define EPAPER_CS           4
    #define EPAPER_SCL          3
    #define EPAPER_SDA          2
#endif

#ifdef ESP32_C3_DIY_LoRa        // just testing!
    #define OLED_SDA         8
    #define OLED_SCL         9
    #define OLED_RST         10
    #define RADIO_SCLK_PIN   4
    #define RADIO_MISO_PIN   5
    #define RADIO_MOSI_PIN   6
    #define RADIO_CS         7
    #define RADIO_RST_PIN    3
    #define RADIO_IRQ_PIN    2
#endif

#if defined(ESP32_C3_OctopusLab_LoRa)
    #define OLED_SDA            0
    #define OLED_SCL            1
    #define OLED_RST            -1
    #define RADIO_SCLK_PIN      6
    #define RADIO_MISO_PIN      4
    #define RADIO_MOSI_PIN      7
    #define RADIO_CS_PIN        5
    #define RADIO_DIO1_PIN      3
    #define RADIO_RST_PIN       -1
    #define RADIO_BUSY_PIN      8
#endif

#ifdef LIGHTGATEWAY_1_0
    #define BUTTON_PIN          0
    #define INTERNAL_LED_PIN    16
#endif


//      GPS
#if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_915) || defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_915) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
    #define GPS_RX              12
    #define GPS_TX              34
#endif

#if defined( HELTEC_WIRELESS_TRACKER)
    #define GPS_RX              34
    #define GPS_TX              33
#endif


/* (Same pins for LILYGO LoRa32 and ESP32 Wroom Dev )
SX1278-------------------> ESP32 ttgo-lora32-v21 and ESP32 WROOM Dev
GND                         GND
DIO1                        -
DIO2                        -
DIO3                        -
VCC                         3.3V
MISO                        19
MOSI                        27
SCLK                        5
NSS                         18
DIO0                        26
REST                        14
GND                         -  */

#endif