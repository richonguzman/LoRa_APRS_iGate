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

    //  LoRa Radio
    #define HAS_SX1268
    #define HAS_1W_LORA
    #define RADIO_SCLK_PIN          18
    #define RADIO_MISO_PIN          19
    #define RADIO_MOSI_PIN          23
    #define RADIO_CS_PIN            5
    #define RADIO_RST_PIN           27
    #define RADIO_DIO1_PIN          12
    #define RADIO_BUSY_PIN          14
    #define RADIO_RXEN              32
    #define RADIO_TXEN              25
    //Uncomment and comment RADIO_TXEN if connect DIO2 to TXEN
    //#define SX126X_DIO2_AS_RF_SWITCH
    //Uncomment if using E22-400M30S
    //#define SX126X_DIO3_TCXO_VOLTAGE 2.2
    #define RADIO_WAKEUP_PIN        RADIO_DIO1_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_12

    //  I2C
    #define USE_WIRE_WITH_OLED_PINS

    //  Display
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                21
    #define OLED_SCL                22
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)

    //  Aditional Config
    #define INTERNAL_LED_PIN        2
    //Uncomment if using INA219
    //#define HAS_INA219
    //#define INA219_ADDR             0x40

#endif