#include "battery_utils.h"
#include "pins_config.h"

extern String   batteryVoltage;

float adcReadingTransformation = (4095/3.3);

namespace BATTERY_Utils {

String checkVoltages() {
    float sample;
    int sampleSum = 0;
    for (int i=0; i<100; i++) {
        sample = analogRead(batteryPin);
        sampleSum += sample;
        delayMicroseconds(50); 
    }
    batteryVoltage = 2.1571 *(sampleSum/100) * adcReadingTransformation;
    return String(batteryVoltage);
}

}