#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ETH.h>
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
    bool _ethernetMode = false;
    bool _ethernetConnected = false;
    unsigned long _apStartup = 0;
    unsigned long _apTimeout = 0;

    String _hostName = "";
    std::vector<WiFiNetwork> _wifiNetworks;

    int _findWiFiNetworkIndex(const String& ssid) const;
    bool _connectWiFi(const WiFiNetwork& network);
    void _processAPTimeout();
    void _onNetworkEvent(arduino_event_id_t event, arduino_event_info_t /*info*/);

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
    bool hasWiFiNetworks() const;
    size_t getWiFiNetworkCount() const;
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

    // Ethernet methods
    bool ethernetConnect(eth_phy_type_t type, uint8_t phy_addr, uint8_t mdc, uint8_t mdio, int power, eth_clock_mode_t clock_mode, bool use_mac_from_efuse = false);
    bool setEthernetIP(const String& staticIP, const String& gateway, const String& subnet, const String& dns1, const String& dns2);
    bool ethernetDisconnect();
    IPAddress getEthernetIP() const;
    String getEthernetMACAddress() const;

    // Check if any network is available
    bool isConnected() const;

    // Check if specific network is connected
    bool isWiFiConnected() const;
    bool isEthernetConnected() const;
    bool isModemConnected() const;

    bool isWifiAPActive() const;
};
