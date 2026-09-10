#include <Arduino.h>
#include <APRSPacketLib.h>
#include <configuration.h>
#include <digi_utils.h>

#include <cstdint>
#include <cstdio>

Configuration Config;
APRSPacket lastAprsPacket;
bool backupDigiMode = false;
uint32_t lastScreenOn = 0;
String iGateBeaconPacket;
String firstLine;
String secondLine;
String thirdLine;
String fourthLine;
String fifthLine;
String sixthLine;
String seventhLine;

namespace STATION_Utils {
bool isIn25SegHashBuffer(const String&, const String&) { return false; }
void updateLastHeard(const String&) {}
void addToOutputPacketBuffer(const String&, bool) {}
}

namespace APRS_IS_Utils {
String checkForStartingBytes(const String& packet) { return packet; }
bool processReceivedLoRaMessage(const String&, const String&) { return false; }
}

namespace Utils {
bool callsignIsValid(const String&) { return true; }
void updateLoRaPacketDisplayInfo(const APRSPacket&, uint8_t) {}
}

void displayToggle(bool) {}

namespace {
int passed = 0;
int failed = 0;
constexpr const char* NONE = "<not digipeated>";

struct Options {
    int txFreq = 433775000;
    int rxFreq = 433775000;
    const char* regionalAliases = "WIDE";
    int regionalMaxHops = 2;
};

void check(const char* label, int mode, const char* packet,
           const char* expected, Options options = Options()) {
    Config.callsign = "F4MLV-10";
    Config.tacticalCallsign = "";
    Config.digi.mode = mode;
    Config.digi.regionalAliases = options.regionalAliases;
    Config.digi.regionalMaxHops = options.regionalMaxHops;
    Config.loramodule.txFreq = options.txFreq;
    Config.loramodule.rxFreq = options.rxFreq;
    lastAprsPacket.header = "";

    String output = DIGI_Utils::generateDigipeatedPacket(String(packet));
    String actual = output.length() ? output : String(NONE);
    bool ok = actual == String(expected);

    std::printf("%s  mode %d  %s\n", ok ? "PASS" : "FAIL", mode, label);
    if (ok) {
        ++passed;
        return;
    }

    std::printf("        in       : %s\n"
                "        expected : %s\n"
                "        got      : %s\n",
                packet, expected, actual.c_str());
    ++failed;
}
}

int main() {
    std::printf("digi_utils.cpp - digipeated path output\n\n");

    std::printf("-- mode 1 : WIDE1-1 fill-in --\n");
    check("WIDE1-1 consumed", 1,
          "F4MLV-7>APLRT1,WIDE1-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*:=/8gk=NmQF[LWQ");
    check("WIDE1-1 already marked -> refused", 1,
          "F4MLV-7>APLRT1,F6DEV-10*,WIDE1-1:=/8gk=NmQF[LWQ", NONE);
    check("WIDE2 only -> not this mode", 1,
          "F4MLV-7>APLRT1,WIDE2-1:=/8gk=NmQF[LWQ", NONE);
    check("no path at all", 1,
          "F4MLV-7>APLRT1:=/8gk=NmQF[LWQ", NONE);

    std::printf("\n-- mode 2 : WIDE1-1 + WIDE2-n --\n");
    check("WIDE1-1 first", 2,
          "F4MLV-7>APLRT1,WIDE1-1,WIDE2-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,WIDE2-1:=/8gk=NmQF[LWQ");
    check("WIDE2-1 alone", 2,
          "F4MLV-7>APLRT1,WIDE2-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*:=/8gk=NmQF[LWQ");
    check("WIDE2-2 decremented", 2,
          "F4MLV-7>APLRT1,WIDE2-2:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,WIDE2-1:=/8gk=NmQF[LWQ");
    check("WIDE2 before WIDE1 -> refused", 2,
          "F4MLV-7>APLRT1,WIDE2-1,WIDE1-1:=/8gk=NmQF[LWQ", NONE);
    check("no WIDE left", 2,
          "F4MLV-7>APLRT1,F6DEV-10*:=/8gk=NmQF[LWQ", NONE);
    check("WIDE1 substring is not an alias", 2,
          "F4MLV-7>APLRT1,XWIDE1-1:=/8gk=NmQF[LWQ", NONE);

    std::printf("\n-- mode 2 : configurable regional aliases --\n");
    Options ariege;
    ariege.regionalAliases = "WIDE ARIEG";
    check("ARIEG1-1 consumed", 2,
          "F4MLV-7>APLRT1,ARIEG1-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*:=/8gk=NmQF[LWQ", ariege);
    check("ARIEG2-2 decremented", 2,
          "F4MLV-7>APLRT1,ARIEG2-2:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,ARIEG2-1:=/8gk=NmQF[LWQ", ariege);
    check("ARIEG2-1 consumed", 2,
          "F4MLV-7>APLRT1,F6DEV-10*,ARIEG2-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F6DEV-10*,F4MLV-10*:=/8gk=NmQF[LWQ", ariege);
    check("fill-in before regional alias", 2,
          "F4MLV-7>APLRT1,WIDE1-1,ARIEG1-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,ARIEG1-1:=/8gk=NmQF[LWQ", ariege);
    check("regional alias before fill-in refused", 2,
          "F4MLV-7>APLRT1,ARIEG1-1,WIDE1-1:=/8gk=NmQF[LWQ", NONE, ariege);
    check("unconfigured alias refused", 2,
          "F4MLV-7>APLRT1,ARIEG1-1:=/8gk=NmQF[LWQ", NONE);
    check("hop count above configured maximum refused", 2,
          "F4MLV-7>APLRT1,ARIEG3-3:=/8gk=NmQF[LWQ", NONE, ariege);
    check("remaining count above initial count refused", 2,
          "F4MLV-7>APLRT1,ARIEG1-2:=/8gk=NmQF[LWQ", NONE, ariege);
    check("consumed regional alias refused", 2,
          "F4MLV-7>APLRT1,ARIEG2-1*:=/8gk=NmQF[LWQ", NONE, ariege);
    Options threeHops = ariege;
    threeHops.regionalMaxHops = 3;
    check("configured WIDEn-N above two hops", 2,
          "F4MLV-7>APLRT1,WIDE3-3:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,WIDE3-2:=/8gk=NmQF[LWQ", threeHops);
    check("configured regional n-N above two hops", 2,
          "F4MLV-7>APLRT1,ARIEG3-3:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,ARIEG3-2:=/8gk=NmQF[LWQ", threeHops);

    std::printf("\n-- mode 2 : exhausted hops (issue #443) --\n");
    check("WIDE2-1 already used", 2,
          "F4MLV-7>APLRT1,F6DEV-10*,WIDE2-1*:=/8gk=NmQF[LWQ", NONE);
    check("reported frame: earlier '*' + used WIDE2-1", 2,
          "F1ZCJ-3>APFD11,F6DEV*,F6DEV-11,WIDE2-1*,WIDE3-3:!4321.45N/00225.50E#",
          NONE);
    check("two consumed hops, WIDE2-1 free", 2,
          "F4MLV-7>APLRT1,F6DEV*,F6DEV-11*,WIDE2-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F6DEV*,F6DEV-11*,F4MLV-10*:=/8gk=NmQF[LWQ");

    std::printf("\n-- mode 3 : own callsign in path --\n");
    check("own callsign present", 3,
          "F4MLV-7>APLRT1,F4MLV-10,WIDE2-1:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F4MLV-10*,WIDE2-1:=/8gk=NmQF[LWQ");
    check("own callsign already marked", 3,
          "F4MLV-7>APLRT1,F4MLV-10*,WIDE2-1:=/8gk=NmQF[LWQ", NONE);
    check("own callsign absent", 3,
          "F4MLV-7>APLRT1,WIDE1-1,WIDE2-1:=/8gk=NmQF[LWQ", NONE);

    std::printf("\n-- cross-frequency digi (tx/rx >= 125 kHz apart) --\n");
    Options crossFrequency;
    crossFrequency.txFreq = 433900000;
    crossFrequency.rxFreq = 433775000;
    check("no WIDE, cross-freq", 1,
          "F4MLV-7>APLRT1,F6DEV-10*:=/8gk=NmQF[LWQ",
          "F4MLV-7>APLRT1,F6DEV-10*,F4MLV-10*:=/8gk=NmQF[LWQ",
          crossFrequency);

    std::printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
