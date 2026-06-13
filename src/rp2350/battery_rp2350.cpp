/*
 * RP2350 supply-voltage monitor. Reads VSYS through the board's 3:1 divider on
 * GP29/ADC3 (Pico2 / W5500-EVB-Pico2): the pin sees VSYS/3, ADC full scale is
 * the 3.3 V reference. analogRead is cheap and not tied to any bus, so it is safe
 * to call from whichever task builds the beacon/telemetry.
 */
#include "battery_rp2350.h"

#ifndef PIN_VSYS_ADC
#define PIN_VSYS_ADC A3        // GP29 = ADC3 = VSYS/3 on Pico2 / W5500-EVB-Pico2
#endif

namespace Battery {

void setup() {
    analogReadResolution(12);
}

float vsys() {
    int raw = analogRead(PIN_VSYS_ADC);          // 0..4095 over 0..3.3 V
    return (float)raw * 3.3f / 4096.0f * 3.0f;   // undo the on-board /3 divider
}

}  // namespace Battery
