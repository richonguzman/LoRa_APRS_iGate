#include "configuration.h"
#include "display.h"

Configuration::Configuration() {
    _filePath = "/igate_conf.json";
    if (!SPIFFS.begin(false)) {
      Serial.println("SPIFFS Mount Failed");
      return;
    }
    readFile(SPIFFS, _filePath.c_str());
}

void Configuration::readFile(fs::FS &fs, const char *fileName) {
    StaticJsonDocument<1536> data;
    File configFile = fs.open(fileName, "r");
    DeserializationError error = deserializeJson(data, configFile);
    if (error) {
        Serial.println("Failed to read file, using default configuration");
    }

    JsonArray WiFiArray = data["wifi"]["AP"];
    for (int i = 0; i < WiFiArray.size(); i++) {
      WiFi_AP wifiap;
      wifiap.ssid                   = WiFiArray[i]["ssid"].as<String>();
      wifiap.password               = WiFiArray[i]["password"].as<String>();
      wifiap.latitude               = WiFiArray[i]["latitude"].as<double>();
      wifiap.longitude              = WiFiArray[i]["longitude"].as<double>();

      wifiAPs.push_back(wifiap);
    }

    callsign                        = data["callsign"].as<String>();
    stationMode                     = data["stationMode"].as<int>();
    iGateComment                    = data["iGateComment"].as<String>();
    beaconInterval                  = data["other"]["beaconInterval"].as<int>();
    rememberStationTime             = data["other"]["rememberStationTime"].as<int>();
    statusAfterBoot                 = data["other"]["statusAfterBoot"].as<bool>();
    defaultStatus                   = data["other"]["defaultStatus"].as<String>();

    digi.comment                    = data["digi"]["comment"].as<String>();
    digi.latitude                   = data["digi"]["latitude"].as<double>();
    digi.longitude                  = data["digi"]["longitude"].as<double>();

    aprs_is.passcode                = data["aprs_is"]["passcode"].as<String>();
    aprs_is.server                  = data["aprs_is"]["server"].as<String>();
    aprs_is.port                    = data["aprs_is"]["port"].as<int>();
    aprs_is.reportingDistance       = data["aprs_is"]["reportingDistance"].as<int>();

    loramodule.iGateFreq            = data["lora"]["iGateFreq"].as<long>();
    loramodule.digirepeaterTxFreq   = data["lora"]["digirepeaterTxFreq"].as<long>();
    loramodule.digirepeaterRxFreq   = data["lora"]["digirepeaterRxFreq"].as<long>();
    loramodule.spreadingFactor      = data["lora"]["spreadingFactor"].as<int>();
    loramodule.signalBandwidth      = data["lora"]["signalBandwidth"].as<long>();
    loramodule.codingRate4          = data["lora"]["codingRate4"].as<int>();
    loramodule.power                = data["lora"]["power"].as<int>();

    display.alwaysOn                = data["display"]["alwaysOn"].as<bool>();
    display.timeout                 = data["display"]["timeout"].as<int>();

    configFile.close();
}

void Configuration::validateConfigFile(String currentBeaconCallsign) {
  if (currentBeaconCallsign == "NOCALL-10") {
    Serial.println("Change Callsign in /data/igate_conf.json");
    show_display("ERROR", "Change your settings", "'igate_conf.json'", "--> File System image", 0);
    while (true) {
      delay(1000);
    }
  }
}