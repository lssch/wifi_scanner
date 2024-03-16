#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <iwlib.h>

class Wifi{
public:
    std::array<int, 12> mac;
    float frequency;
    int channel;
    float channelWidth;
    int strength;
    int quality;

    friend std::ostream& operator<<(std::ostream& os, const Wifi& wifi);
};

class WifiGroup {
public:
    std::string ssid;
    std::list<Wifi> wifi;
    friend std::ostream& operator<<(std::ostream& os, const WifiGroup& group);
};

class WifiScanner {
public:
    explicit WifiScanner(const std::string& interface = "wlan1", bool sort = true, bool ignore_hidden = true);
    ~WifiScanner();

    // Method to perform Wi-Fi scan and store results
    bool scanNetworks();

    void insertWifi(const std::string& ssid, Wifi& wifi);
    void sortWifi();

    friend std::ostream& operator<<(std::ostream& os, const WifiScanner& scan);

private:
    std::string interface_;
    int socket_;
    iwrange range_;
    std::list<WifiGroup> wifis_;

    static int dBmToQuality(int dBm);
    static float estimateChannelWidth(float frequency);
};
