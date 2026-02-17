/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS iGate.
 * 
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    #define HAS_ESP32_S3

    //  LoRa Radio
    #define HAS_SX1262
    #define HAS_TCXO
    #define RADIO_SCLK_PIN          9       // SX1262 SCK
    #define RADIO_MISO_PIN          11      // SX1262 MISO
    #define RADIO_MOSI_PIN          10      // SX1262 MOSI
    #define RADIO_CS_PIN            8       // SX1262 NSS
    #define RADIO_RST_PIN           12      // SX1262 RST
    #define RADIO_DIO1_PIN          14      // SX1262 DIO1
    #define RADIO_BUSY_PIN          13      // SX1262 BUSY
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_14

    //  I2C
    #define USE_WIRE_WITH_BOARD_I2C_PINS
    #define BOARD_I2C_SDA           7
    #define BOARD_I2C_SCL           6
    
    //  Display
    #define HAS_DISPLAY
    #define HAS_TFT

    //  Aditional Config
    #define INTERNAL_LED_PIN        18
    #define BATTERY_PIN             1
    #define VEXT_CTRL               3   // To turn on GPS and TFT
    #define VEXT_CTRL_INVERTED      0
    #define ADC_CTRL                2   // HELTEC Wireless Tracker ADC_CTRL = HIGH powers the voltage divider to read BatteryPin. Only on V05 = V1.1
    #define ADC_CTRL_INVERTED       0

    //  GPS
    #define HAS_GPS
    #define GPS_BAUDRATE            115200
    #define GPS_RX                  34
    #define GPS_TX                  33

#endif