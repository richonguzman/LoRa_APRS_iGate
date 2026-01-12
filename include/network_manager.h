#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>

/**
 * Class for managing network connections
 */
class NetworkManager
{
private:
    bool _wifiAPmode = false;
    bool _wifiSTAmode = false;
    unsigned long _apStartup = 0;
    unsigned long _apTimeout = 0;

    String _generateAPSSID();
    void _processAPTimeout();

public:
    // Constructor
    NetworkManager();

    // Destructor
    ~NetworkManager();

    // Initialize network module
    bool setup();
    void loop();

    // WiFi methods
    bool setupAP(String apName, String apPsk = "");
    bool disableAP();
    void setAPTimeout(unsigned long timeout);
    bool connectWiFi(String ssid, String psk);
    bool disconnectWiFi();
    String getWiFiSSID() const;
    String getWiFiAPSSID() const;
    IPAddress getWiFiIP() const;
    IPAddress getWiFiAPIP() const;
    wifi_mode_t getWiFiMode() const;
    uint8_t* getWiFimacAddress(uint8_t* mac);
    String getWiFimacAddress(void) const;

    // Check if any network is available
    bool isConnected() const;

    // Check if specific network is connected
    bool isWiFiConnected() const;
    bool isEthernetConnected() const;
    bool isModemConnected() const;

    bool isWifiAPActive() const;
};
