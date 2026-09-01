// AUTO-ADAPTED from web_utils.cpp::handleWriteConfiguration (lines 148-293).
// Applies a posted config form (urlencoded OR multipart/form-data — the SPA uses
// fetch + FormData = multipart) to the global Config and persists it. Replaces
// ESPAsyncWebServer's request->getParam/hasParam with a tiny local parser.
// Regenerate if the original mapping changes.
#include <Arduino.h>
#include <vector>
#include "configuration.h"

extern Configuration Config;

struct FormKV { String k, v; };

static String urlDecode(const String &s) {
    String out; out.reserve(s.length());
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '+') out += ' ';
        else if (c == '%' && i + 2 < s.length()) {
            auto hx = [](char h) -> int {
                if (h >= '0' && h <= '9') return h - '0';
                if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                return 0; };
            out += (char)((hx(s[i + 1]) << 4) | hx(s[i + 2])); i += 2;
        } else out += c;
    }
    return out;
}

static void parseUrlencoded(const String &body, std::vector<FormKV> &kv) {
    int i = 0, n = body.length();
    while (i < n) {
        int amp = body.indexOf('&', i); if (amp < 0) amp = n;
        String pair = body.substring(i, amp);
        int eq = pair.indexOf('='); FormKV e;
        if (eq < 0) { e.k = urlDecode(pair); }
        else { e.k = urlDecode(pair.substring(0, eq)); e.v = urlDecode(pair.substring(eq + 1)); }
        if (e.k.length()) kv.push_back(e);
        i = amp + 1;
    }
}

// Minimal multipart/form-data parser for simple text fields (no file uploads).
static void parseMultipart(const String &boundary, const String &body, std::vector<FormKV> &kv) {
    if (!boundary.length()) return;
    String delim = "--" + boundary;
    int pos = 0;
    while (true) {
        int start = body.indexOf(delim, pos);
        if (start < 0) break;
        start += delim.length();
        if (body.substring(start, start + 2) == "--") break;       // closing boundary
        if (body.substring(start, start + 2) == "\r\n") start += 2; // skip CRLF
        int hend = body.indexOf("\r\n\r\n", start);
        if (hend < 0) break;
        String headers = body.substring(start, hend);
        int valStart = hend + 4;
        int next = body.indexOf(delim, valStart);
        if (next < 0) next = body.length();
        int valEnd = next;
        if (valEnd >= 2 && body.substring(valEnd - 2, valEnd) == "\r\n") valEnd -= 2;
        int ni = headers.indexOf("name=\"");
        if (ni >= 0) {
            ni += 6;
            int ne = headers.indexOf('"', ni);
            if (ne > ni) { FormKV e; e.k = headers.substring(ni, ne); e.v = body.substring(valStart, valEnd); kv.push_back(e); }
        }
        pos = next;
    }
}

bool applyConfigForm(const String &contentType, const String &body) {
    std::vector<FormKV> kv;
    if (contentType.indexOf("multipart/form-data") >= 0) {
        int bi = contentType.indexOf("boundary=");
        String boundary = (bi >= 0) ? contentType.substring(bi + 9) : "";
        boundary.trim();
        parseMultipart(boundary, body, kv);
    } else {
        parseUrlencoded(body, kv);
    }
    auto fHas = [&](const String &name) -> bool { for (auto &e : kv) if (e.k == name) return true; return false; };
    auto fGet = [&](const String &name) -> String { for (auto &e : kv) if (e.k == name) return e.v; return String(); };
    auto getParamStringSafe = [&](const String &name, const String &def = "") -> String { return fHas(name) ? fGet(name) : def; };
    auto getParamIntSafe    = [&](const String &name, int def = 0) -> int { return fHas(name) ? fGet(name).toInt() : def; };
    auto getParamFloatSafe  = [&](const String &name, float def = 0.0f) -> float { return fHas(name) ? fGet(name).toFloat() : def; };
    auto getParamDoubleSafe = [&](const String &name, double def = 0.0) -> double { return fHas(name) ? (double)fGet(name).toDouble() : def; };

        int networks = getParamIntSafe("wifi.APs");

        Config.wifiAPs = {};

        for (int i = 0; i < networks; i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = getParamStringSafe("wifi.AP." + String(i) + ".ssid");
            wifiap.password               = getParamStringSafe("wifi.AP." + String(i) + ".password");

            Config.wifiAPs.push_back(wifiap);
        }

        Config.startupDelay                 = getParamIntSafe("startupDelay", Config.startupDelay);

        Config.callsign                     = getParamStringSafe("callsign", Config.callsign);
        Config.tacticalCallsign             = getParamStringSafe("tacticalCallsign", Config.tacticalCallsign);
        Config.wifiAutoAP.enabled           = fHas("wifi.autoAP.enabled");
        Config.wifiAutoAP.password          = getParamStringSafe("wifi.autoAP.password", Config.wifiAutoAP.password);
        Config.wifiAutoAP.timeout           = getParamIntSafe("wifi.autoAP.timeout", Config.wifiAutoAP.timeout);

        Config.aprs_is.active               = fHas("aprs_is.active");
        if (Config.aprs_is.active) {
            Config.aprs_is.messagesToRF     = fHas("aprs_is.messagesToRF");
            Config.aprs_is.objectsToRF      = fHas("aprs_is.objectsToRF");
            Config.aprs_is.server           = getParamStringSafe("aprs_is.server", Config.aprs_is.server);
            Config.aprs_is.passcode         = getParamStringSafe("aprs_is.passcode", Config.aprs_is.passcode);
            Config.aprs_is.port             = getParamIntSafe("aprs_is.port", Config.aprs_is.port);
            Config.aprs_is.filter           = getParamStringSafe("aprs_is.filter", Config.aprs_is.filter);
        }

        Config.beacon.interval              = getParamIntSafe("beacon.interval", Config.beacon.interval);
        Config.beacon.sendViaAPRSIS         = fHas("beacon.sendViaAPRSIS");
        Config.beacon.sendViaRF             = fHas("beacon.sendViaRF");
        Config.beacon.beaconFreq            = getParamIntSafe("beacon.beaconFreq", Config.beacon.beaconFreq);
        Config.beacon.latitude              = getParamDoubleSafe("beacon.latitude", Config.beacon.latitude);
        Config.beacon.longitude             = getParamDoubleSafe("beacon.longitude", Config.beacon.longitude);
        Config.beacon.comment               = getParamStringSafe("beacon.comment", Config.beacon.comment);
        Config.beacon.overlay               = getParamStringSafe("beacon.overlay", Config.beacon.overlay);
        Config.beacon.symbol                = getParamStringSafe("beacon.symbol", Config.beacon.symbol);
        Config.beacon.path                  = getParamStringSafe("beacon.path", Config.beacon.path);

        Config.beacon.statusActive          = fHas("beacon.statusActive");
        if (Config.beacon.statusActive) {
            Config.beacon.statusPacket      = getParamStringSafe("beacon.statusPacket", Config.beacon.statusPacket);
        }

        Config.beacon.gpsActive             = fHas("beacon.gpsActive");
        Config.beacon.ambiguityLevel        = getParamIntSafe("beacon.ambiguityLevel", Config.beacon.ambiguityLevel);

        Config.personalNote                 = getParamStringSafe("personalNote", Config.personalNote);

        Config.blacklist                    = getParamStringSafe("blacklist", Config.blacklist);

        Config.digi.mode                    = getParamIntSafe("digi.mode", Config.digi.mode);
        Config.digi.ecoMode                 = getParamIntSafe("digi.ecoMode", Config.digi.ecoMode);
        Config.digi.backupDigiMode          = fHas("digi.backupDigiMode");

        Config.loramodule.rxActive          = fHas("lora.rxActive");
        Config.loramodule.rxFreq            = getParamIntSafe("lora.rxFreq", Config.loramodule.rxFreq);
        Config.loramodule.rxSpreadingFactor = getParamIntSafe("lora.rxSpreadingFactor", Config.loramodule.rxSpreadingFactor);
        Config.loramodule.rxCodingRate4     = getParamIntSafe("lora.rxCodingRate4", Config.loramodule.rxCodingRate4);
        Config.loramodule.rxSignalBandwidth = getParamIntSafe("lora.rxSignalBandwidth", Config.loramodule.rxSignalBandwidth);
        Config.loramodule.txActive          = fHas("lora.txActive");
        Config.loramodule.txFreq            = getParamIntSafe("lora.txFreq", Config.loramodule.txFreq);
        Config.loramodule.txSpreadingFactor = getParamIntSafe("lora.txSpreadingFactor", Config.loramodule.txSpreadingFactor);
        Config.loramodule.txCodingRate4     = getParamIntSafe("lora.txCodingRate4", Config.loramodule.txCodingRate4);
        Config.loramodule.txSignalBandwidth = getParamIntSafe("lora.txSignalBandwidth", Config.loramodule.txSignalBandwidth);
        Config.loramodule.power             = getParamIntSafe("lora.power", Config.loramodule.power);
        Config.loramodule.cadActive         = fHas("lora.cadActive");

        Config.display.alwaysOn             = fHas("display.alwaysOn");
        if (!Config.display.alwaysOn) {
            Config.display.timeout          = getParamIntSafe("display.timeout", Config.display.timeout);
        }
        Config.display.turn180              = fHas("display.turn180");

        Config.battery.sendInternalVoltage          = fHas("battery.sendInternalVoltage");
        Config.battery.monitorInternalVoltage       = fHas("battery.monitorInternalVoltage");
        if (Config.battery.monitorInternalVoltage) {
            Config.battery.internalSleepVoltage     = getParamFloatSafe("battery.internalSleepVoltage", Config.battery.internalSleepVoltage);
        }

        Config.battery.sendExternalVoltage          = fHas("battery.sendExternalVoltage");
        if (Config.battery.sendExternalVoltage) {
            Config.battery.useExternalI2CSensor     = fHas("battery.useExternalI2CSensor");
        }
        if (Config.battery.sendExternalVoltage) {
            Config.battery.externalVoltagePin       = getParamIntSafe("battery.externalVoltagePin", Config.battery.externalVoltagePin);
            Config.battery.voltageDividerR1         = getParamFloatSafe("battery.voltageDividerR1", Config.battery.voltageDividerR1);
            Config.battery.voltageDividerR2         = getParamFloatSafe("battery.voltageDividerR2", Config.battery.voltageDividerR2);
        }
        Config.battery.monitorExternalVoltage       = fHas("battery.monitorExternalVoltage");
        if (Config.battery.monitorExternalVoltage) {
            Config.battery.externalSleepVoltage     = getParamFloatSafe("battery.externalSleepVoltage", Config.battery.externalSleepVoltage);
        }
        Config.battery.sendVoltageAsTelemetry       = fHas("battery.sendVoltageAsTelemetry");

        Config.wxsensor.active                      = fHas("wxsensor.active");
        if (Config.wxsensor.active) {
            Config.wxsensor.heightCorrection        = getParamIntSafe("wxsensor.heightCorrection", Config.wxsensor.heightCorrection);
            Config.wxsensor.temperatureCorrection   = getParamFloatSafe("wxsensor.temperatureCorrection", Config.wxsensor.temperatureCorrection);
            Config.beacon.symbol = "_";
        }

        Config.syslog.active                    = fHas("syslog.active");
        if (Config.syslog.active) {
            Config.syslog.server                = getParamStringSafe("syslog.server", Config.syslog.server);
            Config.syslog.port                  = getParamIntSafe("syslog.port", Config.syslog.port);
            Config.syslog.logBeaconOverTCPIP    = fHas("syslog.logBeaconOverTCPIP");
        }

        Config.tnc.enableServer             = fHas("tnc.enableServer");
        Config.tnc.enableSerial             = fHas("tnc.enableSerial");
        Config.tnc.acceptOwn                = fHas("tnc.acceptOwn");
        Config.tnc.aprsBridgeActive         = fHas("tnc.aprsBridgeActive");

        Config.mqtt.active                  = fHas("mqtt.active");
        if (Config.mqtt.active) {
            Config.mqtt.server              = getParamStringSafe("mqtt.server", Config.mqtt.server);
            Config.mqtt.topic               = getParamStringSafe("mqtt.topic", Config.mqtt.topic);
            Config.mqtt.username            = getParamStringSafe("mqtt.username", Config.mqtt.username);
            Config.mqtt.password            = getParamStringSafe("mqtt.password", Config.mqtt.password);
            Config.mqtt.port                = getParamIntSafe("mqtt.port", Config.mqtt.port);
            Config.mqtt.beaconOverMqtt      = fHas("mqtt.beaconOverMqtt");
        }

        Config.rebootMode                   = fHas("other.rebootMode");
        if (Config.rebootMode) {
            Config.rebootModeTime           = getParamIntSafe("other.rebootModeTime", Config.rebootModeTime);
        }

        Config.ota.username                 = getParamStringSafe("ota.username", Config.ota.username);
        Config.ota.password                 = getParamStringSafe("ota.password", Config.ota.password);

        Config.webadmin.active              = fHas("webadmin.active");
        if (Config.webadmin.active) {
            Config.webadmin.username        = getParamStringSafe("webadmin.username", Config.webadmin.username);
            Config.webadmin.password        = getParamStringSafe("webadmin.password", Config.webadmin.password);
        }

        Config.remoteManagement.managers    = getParamStringSafe("remoteManagement.managers", Config.remoteManagement.managers);
        Config.remoteManagement.rfOnly      = fHas("remoteManagement.rfOnly");

        Config.ntp.server                   = getParamStringSafe("ntp.server", Config.ntp.server);
        Config.ntp.gmtCorrection            = getParamFloatSafe("ntp.gmtCorrection", Config.ntp.gmtCorrection);

        Config.network.dhcp                 = fHas("network.dhcp");
        if (!Config.network.dhcp) {
            Config.network.ip               = getParamStringSafe("network.ip", Config.network.ip);
            Config.network.gateway          = getParamStringSafe("network.gateway", Config.network.gateway);
            Config.network.subnet           = getParamStringSafe("network.subnet", Config.network.subnet);
            Config.network.dns              = getParamStringSafe("network.dns", Config.network.dns);
        }

        Config.rememberStationTime          = getParamIntSafe("other.rememberStationTime", Config.rememberStationTime);

    Serial.printf("[cfg] applied %d form fields\n", (int)kv.size());
    return Config.writeFile();
}
