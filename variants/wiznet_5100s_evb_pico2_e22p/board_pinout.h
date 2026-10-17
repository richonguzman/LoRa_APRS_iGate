/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
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

    //  WIZnet W5100S-EVB-Pico2 (RP2350 + W5100S on board) + Ebyte E22 / E22P carrier
    //  Hardware: https://github.com/cvaldess/Wiznet_5500_EVB_Pico2_E22P
    //  pico2_w5500_e22, wiznet_5500_evb_pico2_e22p and wiznet_5100s_evb_pico2_e22p
    //  share this pinout: only the Ethernet chip and where it sits change.

    //  LoRa Radio: E22 / E22P, 433 or 868/915 MHz (SX1268 or SX1262, detected at boot)
    #define HAS_SX1262
    #define HAS_1W_LORA
    #define HAS_TCXO
    #define SX126X_DIO3_TCXO_VOLTAGE 1.8
    #define RADIO_SCLK_PIN          10
    #define RADIO_MOSI_PIN          11
    #define RADIO_MISO_PIN          12
    #define RADIO_CS_PIN            13
    #define RADIO_DIO1_PIN          14
    #define RADIO_RST_PIN           15
    #define RADIO_BUSY_PIN          2
    #define RADIO_RXEN              3       // RF enable, held HIGH while the radio is active

    //  TXEN: the DIO2-TXEN jumper ships fitted, so DIO2 keys the PA and no GPIO is
    //  needed. With the jumper moved, TXEN is on GPIO6: build with -D RADIO_TXEN=6.
    #ifndef RADIO_TXEN
        #define RADIO_TXEN          -1
    #endif

    //  Ethernet (SPI0)
    #define PIN_ETH_MISO            16
    #define PIN_ETH_CS              17
    #define PIN_ETH_SCK             18
    #define PIN_ETH_MOSI            19
    #define PIN_ETH_RST             20

    //  I2C: WX sensors (SHT40 on the carrier, STEMMA QT connector)
    #define PIN_WX_SDA              4
    #define PIN_WX_SCL              5

#endif
