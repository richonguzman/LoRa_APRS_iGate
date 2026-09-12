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

#include <Arduino.h>

#include "network_manager.h"

 // Constructor
NetworkManager::NetworkManager() { }

// Destructor
NetworkManager::~NetworkManager() { }

// Private methods

void NetworkManager::_log(const String& text) const {
    if (_logger) {
        _logger(text);
    } else {
        Serial.println(text);
    }
}

int NetworkManager::_findWiFiNetworkIndex(const String& ssid) const {
    for (size_t i = 0; i < _wifiNetworks.size(); i++) {
        if (_wifiNetworks[i].ssid == ssid) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool NetworkManager::_connectWiFi(const WiFiNetwork& network) {
    if (network.ssid.isEmpty()) {
        return false;
    }

    _wifiSTAmode = true;

    if (!_hostName.isEmpty()) {
        WiFi.setHostname(_hostName.c_str());
    }

    WiFi.mode(_wifiAPmode ? WIFI_AP_STA : WIFI_STA);

    _log("[NM] Attempting to connect to WiFi: " + network.ssid);
    WiFi.begin(network.ssid.c_str(), network.psk.c_str());

    // Dots are buffered and logged as one complete line once the loop
    // finishes, rather than streamed live char-by-char as before -- a
    // callback-based logger works in whole lines, not mid-line writes.
    String connectingLine = "[NM] Connecting ";

    int attempts = 0;
    while (!isWiFiConnected() && attempts < 10) {
        delay(500);
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,HIGH);
        #endif
        connectingLine += '.';
        delay(500);
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        attempts++;
    }
    _log(connectingLine);

    if (isWiFiConnected()) return true;

    _log("[NM] Failed to connect to WiFi after " + String(attempts) + " attempts. SSID: " + network.ssid);
    return false;
}

void NetworkManager::_processAPTimeout() {
    if (!_wifiAPmode || _apTimeout == 0) {
        return;
    }

    // If any station is connected, reset the timer
    if (WiFi.softAPgetStationNum() > 0) {
        _apStartup = millis();
        return;
    }

    if (millis() - _apStartup > _apTimeout) {
        _log("[NM] AP timeout reached. Disabling AP mode.");
        disableAP();
    }
}

void NetworkManager::_onNetworkEvent(arduino_event_id_t event, arduino_event_info_t /*info*/) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            _log("[NM] ETH Started");
            if (!_hostName.isEmpty()) {
                _log("[NM] ETH Setting Hostname: " + _hostName);
                ETH.setHostname(_hostName.c_str());
            }
        break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            _log("[NM] ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            _log("[NM] ETH Got IP");
            _ethernetConnected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            _log("[NM] ETH Disconnected");
            _ethernetConnected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            _log("[NM] ETH Stopped");
            _ethernetConnected = false;
            break;
        default:
            break;
    }
}

// Initialize
bool NetworkManager::setup() {
    _log("[NM] Initializing Networking...");

    WiFi.onEvent(
        [this](arduino_event_id_t event, arduino_event_info_t info) {
            _onNetworkEvent(event, info);
        });

    return true;
}

void NetworkManager::loop() {
    if (_wifiAPmode) {
        _processAPTimeout();
    }
}

void NetworkManager::setHostName(const String& hostName) {
    _hostName = hostName;
}

void NetworkManager::setLogger(std::function<void(const String&)> logger) {
    _logger = logger;
}

// WiFi methods

bool NetworkManager::setupAP(String apName, String apPsk) {
    _wifiAPmode = true;

    _log("[NM] Starting AP mode: " + apName);

    // Full WiFi reset sequence
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(200);

    // Set up AP mode with optimized settings
    WiFi.mode(WIFI_AP);

    bool apStarted = WiFi.softAP(apName.c_str(), apPsk.c_str());
    delay(1000); // Give AP time to fully initialize

    if (apStarted) {
        _log("[NM] AP setup successful");
        _apStartup = millis();
    }
    else {
        _log("[NM] AP setup failed");
        return false;
    }

    IPAddress apIP = getWiFiAPIP();
    _log("[NM] AP IP assigned: " + apIP.toString());

    return true;
}

bool NetworkManager::disableAP() {
    WiFi.mode(_wifiSTAmode ? WIFI_STA : WIFI_OFF);
    _wifiAPmode = false;

    return true;
}

void NetworkManager::setAPTimeout(unsigned long timeout) {
    _log("[NM] Setting AP timeout to " + String(timeout / 1000) + " sec");
    _apTimeout = timeout;
}

void NetworkManager::addWiFiNetwork(const String& ssid, const String& psk) {
    if (ssid.isEmpty()) {
        return;
    }

    int index = _findWiFiNetworkIndex(ssid);
    if (index >= 0) {
        _log("[NM] Updating WiFi network: " + ssid);
        _wifiNetworks[static_cast<size_t>(index)].psk = psk;
        return;
    }

    _log("[NM] Adding WiFi network: " + ssid);
    WiFiNetwork network;
    network.ssid = ssid;
    network.psk = psk;
    _wifiNetworks.push_back(network);
}

void NetworkManager::clearWiFiNetworks() {
    _wifiNetworks.clear();
}

bool NetworkManager::hasWiFiNetworks() const {
    return !_wifiNetworks.empty();
}

size_t NetworkManager::getWiFiNetworkCount() const {
    return _wifiNetworks.size();
}

bool NetworkManager::connectWiFi() {
    if (_wifiNetworks.empty()) {
        return false;
    }

    for (size_t i = 0; i < _wifiNetworks.size(); i++) {
        disconnectWiFi();
        if (_connectWiFi(_wifiNetworks[i])) {
            return true;
        }
    }

    return false;
}

bool NetworkManager::connectWiFi(const String& ssid, const String& psk) {
    addWiFiNetwork(ssid, psk);
    return connectWiFi();
}

bool NetworkManager::disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(_wifiAPmode ? WIFI_AP : WIFI_OFF);

    _wifiSTAmode = false;
    return true;
}

String NetworkManager::getWiFiSSID() const {
    return WiFi.SSID();
}

String NetworkManager::getWiFiAPSSID() const {
    return WiFi.softAPSSID();
}

IPAddress NetworkManager::getWiFiIP() const {
    return WiFi.localIP();
}

IPAddress NetworkManager::getWiFiAPIP() const {
    return WiFi.softAPIP();
}

wifi_mode_t NetworkManager::getWiFiMode() const {
    return WiFi.getMode();
}

uint8_t* NetworkManager::getWiFimacAddress(uint8_t* mac) {
    return WiFi.macAddress(mac);
}

String NetworkManager::getWiFimacAddress(void) const {
    return WiFi.macAddress();
}

// Ethernet methods
bool NetworkManager::ethernetConnect(eth_phy_type_t type, uint8_t phy_addr, uint8_t mdc, uint8_t mdio, int power, eth_clock_mode_t clock_mode, bool use_mac_from_efuse) {
    _ethernetMode = true;
    _log("[NM] Setting up Ethernet...");

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        // SDK 5.x (Arduino SDK 3.x)
        #pragma message("Compiling ETH init: SDK 5.x (Arduino core 3.x)")
        return ETH.begin(type, phy_addr, mdc, mdio, power, clock_mode, use_mac_from_efuse);
    #else
        // SDK 4.x (Arduino SDK 2.x)
        #pragma message("Compiling ETH init: SDK 4.x (Arduino core 2.x)")
        return ETH.begin(phy_addr, power, mdc, mdio, type, clock_mode, use_mac_from_efuse);
    #endif
}

bool NetworkManager::setEthernetIP(const String& staticIP, const String& gateway, const String& subnet, const String& dns1, const String& dns2) {
    if (staticIP.isEmpty()) {
        return false;
    }

    IPAddress ip, gw, sn, d1, d2;
    if (!ip.fromString(staticIP) || !gw.fromString(gateway) || !sn.fromString(subnet)) {
        _log("[NM] Invalid static IP configuration");
        return false;
    }

    if (!dns1.isEmpty() && d1.fromString(dns1)) {
        if (!dns2.isEmpty() && d2.fromString(dns2)) {
            ETH.config(ip, gw, sn, d1, d2);
        } else {
            ETH.config(ip, gw, sn, d1);
        }
    } else {
        ETH.config(ip, gw, sn);
    }

    _log("[NM] Ethernet static IP: " + staticIP);
    return true;
}

IPAddress NetworkManager::getEthernetIP() const {
    return ETH.localIP();
}

String NetworkManager::getEthernetMACAddress() const {
    return ETH.macAddress();
}

// Check if network is available
bool NetworkManager::isConnected() const {
    return isWiFiConnected() || isEthernetConnected() || isModemConnected();
}

// Check if WiFi is connected
bool NetworkManager::isWiFiConnected() const {
    return _wifiSTAmode ? WiFi.status() == WL_CONNECTED : false;
}

bool NetworkManager::isWifiAPActive() const {
    return _wifiAPmode;
}

// Check if Ethernet is connected
bool NetworkManager::isEthernetConnected() const {
    return _ethernetMode && _ethernetConnected && ETH.linkUp();
}

// Check if Modem is connected
bool NetworkManager::isModemConnected() const {
    // Implement Modem connection check logic here
    return false;
}
