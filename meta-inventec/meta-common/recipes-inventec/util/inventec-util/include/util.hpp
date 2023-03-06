#pragma once

#include <iomanip>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#define INTF_NAME_MAX_LEN 50
#define MAC_UTIL_MAC_LEN  6

typedef struct IntfInfoStruct
{
    char name[INTF_NAME_MAX_LEN];
    int bus;
    int address;
    int offset;
} IntfInfo;

std::string getEepromPath(size_t bus, size_t address)
{
    std::stringstream output;
    output << "/sys/bus/i2c/devices/" << bus << "-" << std::right
            << std::setfill('0') << std::setw(4) << std::hex << address
            << "/eeprom";
    return output.str();
}

// Calculate new checksum for fru info area
uint8_t calculateChecksum(std::vector<uint8_t>::const_iterator iter,
                            std::vector<uint8_t>::const_iterator end)
{
    constexpr int checksumMod = 256;
    constexpr uint8_t modVal = 0xFF;
    int sum = std::accumulate(iter, end, 0);
    int checksum = (checksumMod - sum) & modVal;
    return static_cast<uint8_t>(checksum);
}

uint8_t calculateChecksum(std::vector<uint8_t> &rawData)
{
    return calculateChecksum(rawData.begin(), rawData.end());
}
