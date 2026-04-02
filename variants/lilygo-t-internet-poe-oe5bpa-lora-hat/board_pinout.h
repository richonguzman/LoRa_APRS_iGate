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

// LILYGO T-Internet-PoE (ESP32-WROVER) + OE5BPA LoRa HAT (SX1278, SSD1306, 2 buttons)
// Wired Ethernet via LAN8720 (PoE capable) as primary uplink; WiFi retained as fallback / WebConfig AP.
// Env: lilygo-t-internet-poe-oe5bpa-lora-hat
// Discussion: https://github.com/richonguzman/LoRa_APRS_iGate/discussions/235

#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

    // LoRa Radio – SX1278 on OE5BPA HAT
    // NOTE: GPIO12 is a boot-strapping pin (selects flash voltage).
    //       The OE5BPA HAT ties CS through a series resistor so the line is
    //       not driven HIGH during reset; verify the HAT has no hard pull-up on CS.
    // NOTE: GPIO36 (SENSOR_VP) and GPIO39 (SENSOR_VN) are input-only;
    //       they have no internal pull-up or pull-down.
    #define HAS_SX1278
    #define RADIO_SCLK_PIN          14
    #define RADIO_MISO_PIN          2
    #define RADIO_MOSI_PIN          15
    #define RADIO_CS_PIN            12      // Strapping pin – see note above
    #define RADIO_RST_PIN           4
    #define RADIO_BUSY_PIN          36      // DIO0 on SX1278 (GPIO36/SENSOR_VP, input-only)
    #define RADIO_WAKEUP_PIN        RADIO_BUSY_PIN
    #define GPIO_WAKEUP_PIN         GPIO_SEL_36

    // I2C – non-default pins, Wire.begin() handled in power_utils.cpp
    #define USE_WIRE_WITH_OLED_PINS

    // Display – SSD1306 OLED on OE5BPA HAT
    #define HAS_DISPLAY

    #undef  OLED_SDA
    #undef  OLED_SCL
    #undef  OLED_RST

    #define OLED_SDA                33
    #define OLED_SCL                32
    #define OLED_RST                -1      // Reset pin # (or -1 if sharing Arduino reset pin)

    // Buttons on OE5BPA HAT
    // SW1 shares the boot-pin (GPIO0) which has an internal pull-up → active LOW.
    // SW2 is on GPIO35 (input-only, no internal pull-up) → active HIGH (button pulls to VCC).
    #define BUTTON_PIN              0       // SW1 – GPIO0 (boot pin, active LOW, internal pull-up)
    #define BUTTON2_PIN             35      // SW2 – GPIO35 (input-only, active HIGH)

    // Wired Ethernet – LAN8720 with PoE
    // ETH_PHY_NRST (GPIO5) is passed as the power/reset pin to ETH.begin() so that
    // the Arduino ETH library can cycle the PHY reset line during initialisation.
    // ETH reference clock is output on GPIO17 (ETH_CLOCK_GPIO17_OUT, mode 3).
    #define HAS_ETHERNET
    #define ETHERNET_PHY_MDC        23
    #define ETHERNET_PHY_MDIO       18
    #define ETHERNET_PHY_NRST       5       // LAN8720 hardware reset (active-LOW, driven by ETH library)
    #define ETHERNET_PHY_ADDR       0

#endif
