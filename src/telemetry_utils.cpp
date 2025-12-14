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

#include <APRSPacketLib.h>
#include <Arduino.h>
#include <vector>
#include "telemetry_utils.h"
#include "aprs_is_utils.h"
#include "configuration.h"
#include "station_utils.h"
#include "battery_utils.h"
#include "lora_utils.h"
#include "wx_utils.h"
#include "display.h"


extern  Configuration       Config;
extern  bool                sendStartTelemetry;

int     telemetryCounter    = random(1,999);


namespace TELEMETRY_Utils {

    String joinWithCommas(const std::vector<String>& items) {
        String result;
        for (size_t i = 0; i < items.size(); ++i) {
            result += items[i];
            if (i < items.size() - 1) result += ",";
        }
        return result;
    }

    std::vector<String> getEquationCoefficients() {
        std::vector<String> coefficients;
        if (Config.battery.sendInternalVoltage) coefficients.push_back("0,0.01,0");
        if (Config.battery.sendExternalVoltage) coefficients.push_back("0,0.02,0");
        return coefficients;
    }

    std::vector<String> getUnitLabels() {
        std::vector<String> labels;
        if (Config.battery.sendInternalVoltage) labels.push_back("VDC");
        if (Config.battery.sendExternalVoltage) labels.push_back("VDC");
        return labels;
    }

    std::vector<String> getParameterNames() {
        std::vector<String> names;
        if (Config.battery.sendInternalVoltage) names.push_back("V_Batt");
        if (Config.battery.sendExternalVoltage) names.push_back("V_Ext");
        return names;
    }

    void sendBaseTelemetryPacket(const String& prefix, const std::vector<String>& values) {
        String packet = prefix + joinWithCommas(values);

        if (Config.beacon.sendViaAPRSIS) {
            String baseAPRSISTelemetryPacket = APRSPacketLib::generateMessagePacket(Config.callsign, "APLRG1", "TCPIP,qAC", Config.callsign, packet);
            #ifdef HAS_A7670
                A7670_Utils::uploadToAPRSIS(baseAPRSISTelemetryPacket);
            #else
                APRS_IS_Utils::upload(baseAPRSISTelemetryPacket);
            #endif
            delay(300);
        } else if (Config.beacon.sendViaRF) {
            String baseRFTelemetryPacket = APRSPacketLib::generateMessagePacket(Config.callsign, "APLRG1", Config.beacon.path, Config.callsign, packet);
            LoRa_Utils::sendNewPacket(baseRFTelemetryPacket);
            delay(3000);
        }
    }

    void sendEquationsUnitsParameters() {
        sendBaseTelemetryPacket("EQNS.", getEquationCoefficients());
        sendBaseTelemetryPacket("UNIT.", getUnitLabels());
        sendBaseTelemetryPacket("PARM.", getParameterNames());
        sendStartTelemetry = false;
    }

    String generateEncodedTelemetryBytes(float value, bool counterBytes, byte telemetryType) {
        String encodedBytes;
        int tempValue;

        if (counterBytes) {
            tempValue = value;
        } else {
            switch (telemetryType) {
                case 0: tempValue = value * 100; break;         // Internal voltage (0-4,2V), Humidity, Gas calculation
                case 1: tempValue = (value * 100) / 2; break;   // External voltage calculation (0-15V)
                case 2: tempValue = (value * 10) + 500; break;  // Temperature
                case 3: tempValue = (value * 8); break;         // Pressure
                default: tempValue = value; break;
            }
        }        

        int firstByte   = tempValue / 91;
        tempValue       -= firstByte * 91;

        encodedBytes    = char(firstByte + 33);
        encodedBytes    += char(tempValue + 33);
        return encodedBytes;
    }

    String generateEncodedTelemetry() {
        String telemetry = "|";
        telemetry += generateEncodedTelemetryBytes(telemetryCounter, true, 0);
        telemetryCounter++;
        if (telemetryCounter == 1000) telemetryCounter = 0;
        if (Config.battery.sendInternalVoltage) telemetry += generateEncodedTelemetryBytes(BATTERY_Utils::checkInternalVoltage(), false, 0);
        if (Config.battery.sendExternalVoltage) telemetry += generateEncodedTelemetryBytes(BATTERY_Utils::checkExternalVoltage(), false, Config.battery.useExternalI2CSensor ? 0 : 1);
        telemetry += "|";
        return telemetry;
    }

}