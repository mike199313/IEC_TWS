#pragma once

#include <cstdint>
#include <map>
#include <numeric>

#define INTF_NAME_MAX_LEN 50
#define ENUM_NAME(val) #val

#define OFFSET_BIOS_CONFIG 0
#define OFFSET_BIOS_CHOSEN 1
#define OFFSET_BIOS_LIST 2
#define OFFSET_CPU_NUM 0
#define OFFSET_CPU_INDEX 1
#define OFFSET_DIMM_NUM 0
#define OFFSET_DIMM_INDEX 1
#define OFFSET_PCIE_NUM 0
#define OFFSET_PCIE_INDEX 1
#define OFFSET_NIC_NUM 0
#define OFFSET_NIC_INDEX 1
#define CPU_ENTRY_SIZE 4
#define CPU_BLOCK_SIZE (CPU_ENTRY_SIZE + 1)
#define DIMM_ENTRY_SIZE 8
#define DIMM_BLOCK_SIZE (DIMM_ENTRY_SIZE + 1)
#define PCIE_ENTRY_SIZE 8
#define PCIE_BLOCK_SIZE (PCIE_ENTRY_SIZE + 1)
#define NIC_ENTRY_SIZE 6
#define NIC_BLOCK_SIZE (NIC_ENTRY_SIZE + 1)
#define MAX_NIC_NUM 2

static constexpr char const* eepromObject = "com.inventec.EepromDevice";
static constexpr char const* eepromPath = "/com/inventec/EepromDevice";
static constexpr char const* eepromMsIntf = "com.inventec.MsEepromManager";

enum AreaID
{
    CPU,
    DIMM,
    PCIE,
    BIOS,   
    NIC
};

const char* AreaNames[] =
{
    ENUM_NAME(CPU),
    ENUM_NAME(DIMM),
    ENUM_NAME(PCIE),
    ENUM_NAME(BIOS),
    ENUM_NAME(NIC)
};

typedef struct eepromArea
{
    uint16_t offset;
    uint16_t size;
    bool checksum;
} EepromArea;

std::map<AreaID, EepromArea> eepromMap
{
    {CPU, {0x600, 0x50, false}},
    {DIMM, {0x650, 0x180, false}},
    {PCIE, {0x7d0, 0x130, false}},
    {BIOS, {0x900, 0x60, true}},
    {NIC, {0x400, 0x0c, true}}
};

// Calculate new checksum for fru info area
char calculateChecksum(const EepromArea& area, char *data)
{
    constexpr int checksumMod = 256;
    constexpr uint8_t modVal = 0xFF;
    int sum = std::accumulate(data, data + area.size, 0);
    int checksum = (checksumMod - sum) & modVal;
    return static_cast<char>(checksum);
}
