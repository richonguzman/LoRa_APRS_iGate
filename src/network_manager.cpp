#include <Arduino.h>

#include "network_manager.h"
 
 // Constructor
NetworkManager::NetworkManager() { }

// Destructor
NetworkManager::~NetworkManager() { }

// Initialize
bool NetworkManager::setup() {
    Serial.println("Initializing Networking...");
    return true;
}


void NetworkManager::loop() {
    if (_wifiAPmode) {
        _processAPTimeout();
    }
}


// WiFi methods

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
        Serial.println("AP timeout reached. Disabling AP mode.");
        disableAP();
    }
}

bool NetworkManager::setupAP(String apName, String apPsk) {
    _wifiAPmode = true;

    Serial.println("Starting AP mode: " + apName);

    // Full WiFi reset sequence
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(200);

    // Set up AP mode with optimized settings
    WiFi.mode(WIFI_AP);

    bool apStarted = WiFi.softAP(apName.c_str(), apPsk.c_str());
    delay(1000); // Give AP time to fully initialize

    if (apStarted) {
        Serial.println("AP setup successful");
        _apStartup = millis();
    }
    else {
        Serial.println("AP setup failed");
        return false;
    }

    IPAddress apIP = getWiFiAPIP();
    Serial.println("AP IP assigned: " + apIP.toString());

    return true;
}

bool NetworkManager::disableAP() {
    WiFi.mode(_wifiSTAmode ? WIFI_STA : WIFI_OFF);
    _wifiAPmode = false;

    return true;
}

void NetworkManager::setAPTimeout(unsigned long timeout) {
    Serial.println("Setting AP timeout to " + String(timeout / 1000) + " sec");
    _apTimeout = timeout;
}

bool NetworkManager::connectWiFi(String ssid, String psk) {
    _wifiSTAmode = true;

    WiFi.mode(_wifiAPmode ? WIFI_AP_STA : WIFI_STA);

    Serial.println("Attempting to connect to WiFi: " + ssid);
    WiFi.begin(ssid.c_str(), psk.c_str());

    Serial.print("Connecting ");

    int attempts = 0;
    while (!isWiFiConnected() && attempts < 10) {
        delay(500);
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,HIGH);
        #endif
        Serial.print('.');
        delay(500);
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN,LOW);
        #endif
        attempts++;
    }
    Serial.println();

    if (isWiFiConnected()) {
        Serial.println("WiFi connected! IP: " + WiFi.localIP().toString());
        return true;
    }
    else {
        Serial.println("Failed to connect to WiFi after " + String(attempts) + " attempts. SSID: " +
                            ssid);
        return false;
    }
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
    // Implement Ethernet connection check logic here
    return false;
}

// Check if Modem is connected
bool NetworkManager::isModemConnected() const {
    // Implement Modem connection check logic here
    return false;
}
