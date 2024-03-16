cmake_minimum_required(VERSION 3.10)
project(wifi_test)

set(CMAKE_CXX_STANDARD 20)

# Find libiw
find_library(LIBIW_LIBRARY NAMES iw)

# Check if libiw is found
if(NOT LIBIW_LIBRARY)
    message(FATAL_ERROR "libiw not found")
endif()

add_executable(wifi_test main.cpp WiFiScanner/WiFiScanner.cpp WiFiScanner/WiFiScanner.hpp)

# Link libiw
target_link_libraries(wifi_test ${LIBIW_LIBRARY})
