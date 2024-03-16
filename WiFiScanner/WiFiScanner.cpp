#include "WiFiScanner.hpp"

#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <net/if.h>

WifiScanner::WifiScanner(const std::string& interface, bool sort, bool ignore_hidden) {
    // check interface
    if (if_nametoindex(interface.c_str()) == 0) {
        std::cerr << "Error: Interface '" << interface << "' does not exist" << std::endl;
    }
    interface_ = interface;

    // Open socket
    socket_ = iw_sockets_open();
    if (socket_ < 0) {
        std::cerr << "Error opening socket. Make sure you have the necessary permissions.\n";
    }

    // Get range
    if (iw_get_range_info(socket_, interface_.c_str(), &range_) < 0) {
        std::cerr << "Error getting range information." << std::endl;
        iw_sockets_close(socket_);
    }
}

WifiScanner::~WifiScanner() {
    iw_sockets_close(socket_);
}

bool WifiScanner::scanNetworks() {
    wifis_.clear();
    wireless_scan_head head;

    // Scan for networks
    if (iw_scan(socket_, const_cast<char*>(interface_.c_str()), range_.we_version_compiled, &head) < 0) {
        std::cerr << "Error scanning for networks." << std::endl;
        iw_sockets_close(socket_);
        return false;
    }

    // Populate scan results
    for (wireless_scan *result = head.result; result != NULL; result = result->next) {
        Wifi wifi;
        for (int i = 0; i < wifi.mac.size(); ++i) {
            wifi.mac.at(i) = static_cast<int>(result->ap_addr.sa_data[i]);
        }

        wifi.frequency = result->b.freq;

        // Calculate channel
        wifi.channel = iw_freq_to_channel(result->b.freq, &range_);
        if (wifi.channel < 0) {
            std::cerr << "Error: Failed to get channel for frequency " << result->b.freq << std::endl;
            iw_sockets_close(socket_);
            return false;
        }

        wifi.channelWidth = estimateChannelWidth(wifi.frequency);
        wifi.strength = -result->stats.qual.level;
        wifi.quality = dBmToQuality(wifi.strength);

        if (std::string((char*)result->b.essid).empty()) {
            continue;
            //std::string ssid = "HIDDEN";
            //insertWifi(ssid, wifi);
        } else {
            insertWifi((char*)result->b.essid, wifi);
        }
    }

    free(head.result);

    sortWifi();

    return true;
}

void WifiScanner::insertWifi(const std::string& ssid, Wifi& wifi) {
    // Check if ssid is present
    for (auto & network : wifis_) {
        if (network.ssid == ssid) {
            network.wifi.push_back(wifi);
            return;
        }
    }

    WifiGroup wifi_group;
    wifi_group.ssid = ssid;
    wifi_group.wifi.push_back(wifi);
    wifis_.push_back(wifi_group);
}

void WifiScanner::sortWifi() {
    // Sort individual wifi by signal
    for (auto& group : wifis_) {
        group.wifi.sort([](const Wifi& lhs, const Wifi& rhs) {
            return lhs.strength < rhs.strength;
        });
    }

    // Sort groups by signal strength
    wifis_.sort([](const WifiGroup& lhs, const WifiGroup& rhs) {
        if ((lhs.wifi.empty() and rhs.wifi.empty()) or lhs.wifi.empty()) {
            return false;
        } else if (rhs.wifi.empty()) {
            return true;
        }
        return lhs.wifi.front().strength < rhs.wifi.front().strength;
    });
}

int WifiScanner::dBmToQuality(const int dBm) {
    int worst_dBm = -85;
    int best_dBm = -18;
    int quality = static_cast<int>(100*(static_cast<float>(dBm - best_dBm) / (worst_dBm - best_dBm)));

    if (quality > 100) return 100;
    if (quality < 1) return 1;
    return quality;
}

float WifiScanner::estimateChannelWidth(float frequency) {
    if (frequency < 2.422e9) {
        // Frequencies below 2.422 GHz are typically 20 MHz wide
        return 20e6;
    } else if (frequency < 2.462e9) {
        // Frequencies between 2.422 GHz and 2.462 GHz can be either 20 MHz or 40 MHz wide
        return 40e6;
    } else if (frequency < 2.484e9) {
        // Frequencies between 2.462 GHz and 2.484 GHz are typically 20 MHz wide
        return 20e6;
    } else if (frequency < 5.18e9) {
        // Frequencies below 5.18 GHz are typically 20 MHz wide
        return 20e6;
    } else if (frequency < 5.25e9) {
        // Frequencies between 5.18 GHz and 5.25 GHz are typically 40 MHz wide
        return 40e6;
    } else if (frequency < 5.32e9) {
        // Frequencies between 5.25 GHz and 5.32 GHz are typically 80 MHz wide
        return 80e6;
    } else {
        // Frequencies above 5.32 GHz are typically 160 MHz wide
        return 160e6;
    }
}

std::ostream &operator<<(std::ostream &os, const WifiScanner &scan) {
    os << std::string(105, '-') << std::endl;
    os << std::setw(35) << std::left << "SSID"
       << std::setw(20) << std::left << "MAC"
       << std::setw(10) << std::left << "Frequency"
       << std::setw(10) << std::left << "Channel"
       << std::setw(10) << std::left << "Width"
       << std::setw(10) << std::left << "Strength"
       << std::setw(10) << std::left << "Quality"
       << std::endl;
    os << std::string(105, '-') << std::endl;

    for (const auto& network: scan.wifis_) {
        os << network;
    }

    os << std::string(105, '-') << std::endl;

    return os;
}

std::ostream &operator<<(std::ostream &os, const WifiGroup &group) {
    os << std::setw(35) << std::left << group.ssid;

    auto iter = group.wifi.begin();
    auto end = group.wifi.end();

    for (; iter != end; ++iter) {
        os << *iter << std::endl;
        if (std::next(iter) != end)
            os << std::setw(37) << std::left << " ∟";
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const Wifi &wifi) {
    std::stringstream mac;
    std::stringstream frequency;
    std::stringstream channel_width;
    std::stringstream strength;
    std::stringstream quality;

    mac << std::hex << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        if (i > 0) mac << ':';
        mac << std::setw(2) << static_cast<int>(wifi.mac.at(i));
    }

    frequency << std::setprecision(2) << round(wifi.frequency*1e-8) / 1e1 << " MHz";
    channel_width << wifi.channelWidth*1e-6 << " MHz";
    strength << +wifi.strength << " dBm";
    quality << +wifi.quality << " %";

    os << std::setw(20) << std::left << mac.str()
       << std::setw(10) << std::left << frequency.str()
       << std::setw(10) << std::left << wifi.channel
       << std::setw(10) << std::left << channel_width.str()
       << std::setw(10) << std::left << strength.str()
       << std::setw(10) << std::left << quality.str();

    return os;
}
