#ifndef PINS_CONFIG_H_
#define PINS_CONFIG_H_

#include <Arduino.h>

#define LORA_SCK    5       // GPIO5    - SX1276 SCK
#define LORA_MISO   19      // GPIO19   - SX1276 MISO
#define LORA_MOSI   27      // GPIO27   - SX1276 MOSI
#define LORA_CS     18      // GPIO18   - SX1276 CS ---> NSS
#define LORA_RST    23      // GPIO14   - SX1276 RST
#define LORA_IRQ    26      // GPIO26   - SX1276 IRQ ---->DIO0

#define OLED_SDA 21
#define OLED_SCL 22

/* (Same pins for LILYGO LoRa32 and ESP32 Wroom Dev )
SX1278-------------------> ESP32 TTGO LILYGO 2.1 or 1.6.1
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
GND                         -
*/
#endif
