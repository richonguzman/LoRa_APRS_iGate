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

// Per-board calibration of the VSYS reading. This code was first tuned on the
// WIZnet W5500-EVB-Pico2, whose VSYS sense reads accurately (factor 1.0). On a
// genuine Raspberry Pi Pico 2 the on-board 200k/100k divider on GP29 reads
// consistently low (~0.59x) under the RP2350 ADC, so the e22-2 variant sets
// VSYS_CAL to bring the reported value back to the measured ~5 V supply.
#ifndef VSYS_CAL
#define VSYS_CAL 1.0f
#endif

namespace Battery {

void setup() {
    analogReadResolution(12);
}

float vsys() {
    // The Raspberry Pi Pico 2's on-board VSYS/3 divider (200k/100k) is high
    // impedance, so the first ADC sample after the mux selects this channel reads
    // low (the sample-and-hold cap shares charge with the previous channel/state).
    // The WIZnet W5500-EVB-Pico2 this was first ported on happened to read fine
    // with a single shot; a genuine Pico 2 does not. Discard a couple of priming
    // reads to let the S&H settle on the high-Z source, then average.
    (void)analogRead(PIN_VSYS_ADC);
    (void)analogRead(PIN_VSYS_ADC);
    delayMicroseconds(50);
    uint32_t acc = 0;
    for (int i = 0; i < 16; i++) {
        acc += analogRead(PIN_VSYS_ADC);         // 0..4095 over 0..3.3 V
        delayMicroseconds(20);
    }
    return (float)acc / 16.0f * 3.3f / 4096.0f * 3.0f * VSYS_CAL;   // undo /3 divider + per-board cal
}

}  // namespace Battery
