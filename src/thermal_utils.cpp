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

// Fan / thermal control adapted from KJ7NYE's LoRa_FieldOps_APRS_Tracker (GPLv3)
// NTC circuit (T-Beam 1W V1.0 schematic): 3V3 -> NCP18XH103F03RB -> TEMP_PIN -> 10K -> GND

#include "board_pinout.h"

#ifdef FAN_CTRL_PIN

#include <Arduino.h>
#include <math.h>
#include "thermal_utils.h"
#include "utils.h"

#define NTC_B               3380.0f     // B25/50 (K)
#define NTC_R25             10000.0f    // NTC resistance at 25C (ohm)
#define NTC_T0              298.15f     // 25C in Kelvin
#define NTC_R_FIXED         10000.0f    // pull-down resistor (ohm)
#define NTC_VCC_MV          3300.0f
#define NTC_MIN_VALID_MV    100         // below this the sample is a failed ADC2 read (100mV = ~-45C)

#define FAN_ON_TEMP         50.0f       // fan on at or above
#define FAN_OFF_TEMP        42.0f       // fan off below (hysteresis)
#define TEMP_WARNING        75.0f
#define TEMP_TX_BLOCK       85.0f       // Tx blocked at or above
#define TEMP_TX_UNBLOCK     75.0f       // Tx allowed again below (hysteresis)
#define TX_COOLDOWN_MS      30000       // fan kept on after TX
#define TEMP_SAMPLE_MS      30000

static float       currentTemperature  = 25.0f;
static bool        fanOn               = false;
static bool        txBlocked           = false;
static bool        txCooldownActive    = false;
static uint32_t    txEndTime           = 0;
static uint32_t    lastTempSample      = 0;
static bool        firstTempSample     = true;


namespace THERMAL_Utils {

    float readTemperature() {       // TEMP_PIN is on ADC2: reads fail (return 0) while Wi-Fi holds the ADC2 arbiter
        analogReadMilliVolts(TEMP_PIN);     // dummy read to settle the ADC
        delay(1);
        uint32_t sum = 0;
        int validSamples = 0;
        for (int i = 0; i < 5; i++) {
            uint32_t sample = analogReadMilliVolts(TEMP_PIN);
            if (sample > NTC_MIN_VALID_MV) {
                sum += sample;
                validSamples++;
            }
            delay(3);
        }
        if (validSamples == 0) return NAN;
        float mv = (float)sum / validSamples;
        if (mv >= NTC_VCC_MV) return NAN;
        float ntcResistance = NTC_R_FIXED * (NTC_VCC_MV - mv) / mv;
        float kelvin = 1.0f / (1.0f / NTC_T0 + (1.0f / NTC_B) * logf(ntcResistance / NTC_R25));
        return kelvin - 273.15f;
    }

    void setFan(bool on, const String& reason) {
        if (fanOn == on) return;
        fanOn = on;
        digitalWrite(FAN_CTRL_PIN, on ? HIGH : LOW);
        Utils::println("Thermal: Fan " + String(on ? "ON" : "OFF") + " (" + reason + ", " + String(currentTemperature, 1) + " C)");
    }

    void updateFan() {
        if (!fanOn) {
            if (txCooldownActive) {
                setFan(true, "TX cooldown");
            } else if (currentTemperature >= FAN_ON_TEMP) {
                setFan(true, "temperature");
            }
        } else if (!txCooldownActive && currentTemperature < FAN_OFF_TEMP) {
            setFan(false, "cooled down");
        }
    }

    void setup() {
        pinMode(TEMP_PIN, INPUT);
        analogSetPinAttenuation(TEMP_PIN, ADC_11db);
        pinMode(FAN_CTRL_PIN, OUTPUT);
        digitalWrite(FAN_CTRL_PIN, LOW);
        fanOn           = false;
        firstTempSample = true;     // first monitor() call samples immediately
    }

    void monitor() {
        uint32_t now = millis();

        if (txCooldownActive && (now - txEndTime >= TX_COOLDOWN_MS)) {
            txCooldownActive = false;
            updateFan();
        }

        if (!firstTempSample && (now - lastTempSample < TEMP_SAMPLE_MS)) return;
        firstTempSample = false;
        lastTempSample  = now;

        float temperature = readTemperature();
        if (!isnan(temperature)) currentTemperature = temperature;

        if (!txBlocked && currentTemperature >= TEMP_TX_BLOCK) {
            txBlocked = true;
            Utils::println("Thermal: Over-temperature at " + String(currentTemperature, 1) + " C, Tx blocked");
        } else if (txBlocked && currentTemperature < TEMP_TX_UNBLOCK) {
            txBlocked = false;
            Utils::println("Thermal: Temperature back to " + String(currentTemperature, 1) + " C, Tx allowed");
        } else if (!txBlocked && currentTemperature >= TEMP_WARNING) {
            Utils::println("Thermal: High temperature: " + String(currentTemperature, 1) + " C");
        }
        updateFan();
    }

    void onTxStart() {
        txCooldownActive = false;
        setFan(true, "TX");
    }

    void onTxEnd() {
        txCooldownActive = true;
        txEndTime        = millis();
    }

    void turnOffFan() {     // before sleeping: no Tx while asleep, so the PA doesn't heat up
        txCooldownActive = false;
        setFan(false, "sleep");
    }

    bool isTxBlocked() {
        return txBlocked;
    }

}

#endif
