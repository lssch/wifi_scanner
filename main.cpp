#include "WiFiScanner/WiFiScanner.hpp"
#include <chrono>
#include <thread>

int main(int argc, char** argv) {
    int interval = 1000;
    if (argc > 1) {
        try {
            interval = std::stoi(argv[1]);
        } catch (std::exception &ex) {
            std::cout << "Not a valid interval." << std::endl;
            return 1;
        }
    }

    std::string interface = "wlan1";
    if (argc > 2)
        interface = argv[2];

    try {
        WifiScanner wifi{interface};

        double scan_number = 1;

        while (true) {
            // Perform a scan
            if (!wifi.scanNetworks()) {
                std::cerr << "Error scanning networks. Retrying in " << interval << " milliseconds.\n";
            } else {
                // Print the latest scan results
                system("clear");
                std::cout << wifi;
                std::cout << "Scan number: " << scan_number++ << "; Scan interval: " << interval << std::endl;
            }

            // Wait for the specified interval before scanning again
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }

    } catch(const std::exception &ex) {
        throw std::runtime_error(std::string("Couldn't initialise wifi scanner") + ex.what());
    }

    return 0;
}
