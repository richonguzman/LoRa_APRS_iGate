#ifndef PINS_CONFIG_H_
#define PINS_CONFIG_H_

#include <Arduino.h>

#undef OLED_SDA
#undef OLED_SCL
#undef OLED_RST

#if defined(HELTEC_V3) || defined(ESP32_DIY_1W_LoRa) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
#define HAS_SX126X
#endif

#if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(ESP32_DIY_LoRa) || defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_2) || defined(ESP32_LOLIN_OLED_DIY_LoRa)
#define HAS_SX127X
#endif

#if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_0_SX1268)
#define HAS_AXP192
#endif

#if defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_2_SX1262)
#define HAS_AXP2101
#endif

// LORA MODULES
#if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(ESP32_DIY_LoRa)
#define LORA_SCK 5   // GPIO5    - SX1276 SCK
#define LORA_MISO 19 // GPIO19   - SX1276 MISO
#define LORA_MOSI 27 // GPIO27   - SX1276 MOSI
#define LORA_CS 18   // GPIO18   - SX1276 CS ---> NSS
#define LORA_RST 14  // GPIO14   - SX1276 RST
#define LORA_IRQ 26  // GPIO26   - SX1276 IRQ ---->DIO0
#endif

#ifdef ESP32_LOLIN_OLED_DIY_LoRa
#define LORA_SCK 15  // GPIO5    - SX1276 SCK
#define LORA_MISO 13 // GPIO19   - SX1276 MISO
#define LORA_MOSI 12 // GPIO27   - SX1276 MOSI
#define LORA_CS 14   // GPIO18   - SX1276 CS ---> NSS
#define LORA_RST 2   // GPIO14   - SX1276 RST
#define LORA_IRQ 25  // GPIO26   - SX1276 IRQ ---->DIO0
#endif

#ifdef HELTEC_V3
#define RADIO_SCLK_PIN 9  // SX1262 SCK
#define RADIO_MISO_PIN 11 // SX1262 MISO
#define RADIO_MOSI_PIN 10 // SX1262 MOSI
#define RADIO_CS_PIN 8    // SX1262 NSS
#define RADIO_RST_PIN 12  // SX1262 RST
#define RADIO_DIO1_PIN 14 // SX1262 DIO1
#define RADIO_BUSY_PIN 13 // SX1262 BUSY
#endif

#ifdef ESP32_DIY_1W_LoRa // Ebyte E22 400M30S / SX1268
#define RADIO_SCLK_PIN 18
#define RADIO_MISO_PIN 19
#define RADIO_MOSI_PIN 23
#define RADIO_CS_PIN 5
#define RADIO_RST_PIN 27
#define RADIO_DIO1_PIN 12
#define RADIO_BUSY_PIN 14
#define RADIO_RXEN 32
#define RADIO_TXEN 25
#endif

#if defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
#define RADIO_SCLK_PIN 5
#define RADIO_MISO_PIN 19
#define RADIO_MOSI_PIN 27
#define RADIO_CS_PIN 18
#define RADIO_DIO0_PIN 26
#define RADIO_RST_PIN 23
#define RADIO_DIO1_PIN 33
#define RADIO_BUSY_PIN 32
#endif

// OLED
#if defined(TTGO_T_LORA32_V2_1) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa) || defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_2) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

#ifdef ESP32_LOLIN_OLED_DIY_LoRa
#define OLED_SDA 5
#define OLED_SCL 4
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

#ifdef HELTEC_V2
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RESET 16
#endif

#ifdef HELTEC_V3
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RESET 21
#endif

// Leds and other stuff
#if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2)
#define internalLedPin 25 // Green Led
#define batteryPin 35
#endif
#ifdef HELTEC_V3
#define internalLedPin 35
#endif
#if defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
#define internalLedPin 2
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