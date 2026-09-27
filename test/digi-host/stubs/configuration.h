#pragma once

#include <Arduino.h>

struct Digi {
    int mode = 0;
    int ecoMode = 0;
    bool backupDigiMode = false;
    String regionalAliases = "WIDE";
    int regionalMaxHops = 2;
};

struct LoraModule {
    int txFreq = 433775000;
    int rxFreq = 433775000;
};

struct Configuration {
    String callsign;
    String tacticalCallsign;
    Digi digi;
    LoraModule loramodule;
};
