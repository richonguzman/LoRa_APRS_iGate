#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <vector>

/**
 * Class for managing network connections
 */
class NetworkManager
{
private:
    class WiFiNetwork {
    public:
        String ssid;
        String psk;
    };

    bool _wifiAPmode = false;
    bool _wifiSTAmode = false;
    unsigned long _apStartup = 0;
    unsigned long _apTimeout = 0;

    String _hostName = "";
    std::vector<WiFiNetwork> _wifiNetworks;

    int _findWiFiNetworkIndex(const String& ssid) const;
    bool _connectWiFi(const WiFiNetwork& network);
    void _processAPTimeout();

public:
    // Constructor
    NetworkManager();

    // Destructor
    ~NetworkManager();

    // Initialize network module
    bool setup();
    void loop();

    void setHostName(const String& hostName);

    // WiFi methods
    bool setupAP(String apName, String apPsk = "");
    bool disableAP();
    void setAPTimeout(unsigned long timeout);
    void addWiFiNetwork(const String& ssid, const String& psk = "");
    void clearWiFiNetworks();
    bool connectWiFi();
    bool connectWiFi(const String& ssid, const String& psk = "");
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
