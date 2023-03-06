#include "eeprom_util.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include "mac_util.hpp"
#include <utility>

#include <boost/asio/io_service.hpp>
#include <sdbusplus/asio/object_server.hpp>

static IntfInfo *findIntfInfo(char *intfName)
{
    int intfSize, i;
    IntfInfo *ret = NULL;

    intfSize = sizeof(intfInfoList) / sizeof(IntfInfo);

    for (i = 0; i < intfSize; i++)
    {
        if (strcmp(intfName, intfInfoList[i].name) == 0)
        {
            ret = &intfInfoList[i];
            break;
        }
    }
    return ret;
}

static bool readEeprom(const EepromArea& area, char *buff, bool ignore_check=false)
{
    std::string path = getEepromPath(intfInfoList[0].bus, intfInfoList[0].address);
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

    if (file.is_open())
    {
        // Read from sysfs
        file.seekg(area.offset, std::ios::beg);
        file.read(buff, area.checksum ? area.size + 1 : area.size);
        file.close();

        // Check checksum
        if (area.checksum)
        {
            char checksum = calculateChecksum(area, buff);
            if (!ignore_check && checksum != buff[area.size])
            {
                std::cerr << "Checksum error" << std::endl;
                return false;
            }
        }

        return true;
    }

    std::cerr << "readEeprom Unable to oepn file " << path << std::endl;
    return false;
}

static bool writeEeprom(const EepromArea& area, char *data)
{
    std::string path = getEepromPath(intfInfoList[0].bus, intfInfoList[0].address);
	std::fstream file(path.c_str(), std::ios::in | std::ios::out | std::ios::binary);

	if (file.is_open())
	{
		file.seekp(area.offset, std::ios::beg);

        if (area.checksum)
        {
            // Calaulate checksum
            data[area.size] = calculateChecksum(area, data);
        }

        file.write(data, area.checksum ? area.size + 1 : area.size);
		file.close();
		return true;
	}

	std::cerr << "Unable to open file " << path << std::endl;
    return false;
}

std::vector<uint8_t> getBiosConfig()
{
    EepromArea bios = eepromMap[AreaID::BIOS];
    char buff[bios.size + 1] = {0}; //Include checksum byte
    std::vector<uint8_t> data;
    int size = 2;

    if (readEeprom(bios, buff))
    {
        data.insert(data.begin(), buff, buff + bios.size);
        while(size < bios.size && data[size] != 0)
        {
            size += 4;
        }

        if (size < bios.size)
        {
            data.resize(size);
        }
        return data;
    }

    return data;
}

bool setBiosConfig(const uint8_t& val)
{
    EepromArea bios = eepromMap[AreaID::BIOS];
    char buff[bios.size + 1] = {0}; //Include checksum byte

    if (readEeprom(bios, buff, true))
    {
        buff[OFFSET_BIOS_CHOSEN] = static_cast<char>(val);
        return writeEeprom(bios, buff);
    }

    std::cerr << "setBiosConfig failed to read from EEPROM" << std::endl;
    return false;
}

bool setBiosConfigInfo(const std::vector<uint8_t>& data)
{
    EepromArea bios = eepromMap[AreaID::BIOS];

    if (data.size() > bios.size)
    {
        std::cerr << "setBiosConfigInfo exceeds max size" << std::endl;
        return false;
    }

    char tmp[bios.size + 1] = {0}; //Include checksum byte

    if (!readEeprom(bios, tmp, true))
    {
        return false;
    }

    char buff[bios.size + 1] = {0}; //Include checksum byte
    buff[OFFSET_BIOS_CONFIG] = static_cast<char>(data[0]);
    buff[OFFSET_BIOS_CHOSEN] = tmp[OFFSET_BIOS_CHOSEN];
    auto itStart = data.begin();
    std::advance(itStart, 1);
    std::copy(itStart, data.end(), buff + OFFSET_BIOS_LIST);
    return writeEeprom(bios, buff);
}

std::vector<uint8_t> getCPUInfo(const uint8_t& val)
{
    EepromArea cpu = eepromMap[AreaID::CPU];
    char buff[cpu.size] = {0};
    std::vector<uint8_t> data;

    if (readEeprom(cpu, buff))
    {
        int total = static_cast<int>(buff[OFFSET_CPU_NUM]);
        int offset = OFFSET_CPU_INDEX;
        int num = 0;
        uint8_t idx;

        while (num < total && offset < cpu.size)
        {
            idx = static_cast<uint8_t>(buff[offset]);

            // match found
            if (idx == val)
            {
                offset++;   // skip index byte
                data.insert(data.begin(), &buff[offset], &buff[offset + CPU_ENTRY_SIZE]);
                return data;
            }

            offset += CPU_BLOCK_SIZE;
            num++;
        }
    }

    std::cerr << "getCPUInfo failed to read from EEPROM" << std::endl;
    return data;
}

bool setCPUInfo(const std::vector<uint8_t>& data)
{
    EepromArea cpu = eepromMap[AreaID::CPU];

    if (data.size() > cpu.size)
    {
        std::cerr << "setCPUInfo exceeds max size" << std::endl;
        return false;
    }

    char buff[cpu.size] = {0};
    std::copy(data.begin(), data.end(), buff);
    return writeEeprom(cpu, buff);
}

std::vector<uint8_t> getMemInfo(const uint8_t& val)
{
    EepromArea dimm = eepromMap[AreaID::DIMM];
    char buff[dimm.size] = {0};
    std::vector<uint8_t> data;

    if (readEeprom(dimm, buff))
    {
        int total = static_cast<int>(buff[OFFSET_DIMM_NUM]);
        int offset = OFFSET_DIMM_INDEX;
        int num = 0;
        uint8_t idx;

        while (num < total && offset < dimm.size)
        {
            idx = static_cast<uint8_t>(buff[offset]);

            // match found
            if (idx == val)
            {
                offset++;   // skip index byte
                data.insert(data.begin(), &buff[offset], &buff[offset + DIMM_ENTRY_SIZE]);
                return data;
            }

            offset += DIMM_BLOCK_SIZE;
            num++;
        }
    }

    std::cerr << "getMemInfo failed to read from EEPROM" << std::endl;
    return data;
}

bool setMemInfo(const std::vector<uint8_t>& data)
{
    EepromArea dimm = eepromMap[AreaID::DIMM];

    if (data.size() > dimm.size)
    {
        std::cerr << "setCPUInfo exceeds max size" << std::endl;
        return false;
    }

    char buff[dimm.size] = {0};
    std::copy(data.begin(), data.end(), buff);
    return writeEeprom(dimm, buff);
}

std::vector<uint8_t> getPcieInfo(const uint8_t& val)
{
    EepromArea pcie = eepromMap[AreaID::PCIE];
    char buff[pcie.size] = {0};
    std::vector<uint8_t> data;

    if (readEeprom(pcie, buff))
    {
        int total = static_cast<int>(buff[OFFSET_PCIE_NUM]);
        int offset = OFFSET_PCIE_INDEX;
        int num = 0;
        uint8_t idx;

        while (num < total && offset < pcie.size)
        {
            idx = static_cast<uint8_t>(buff[offset]);

            // match found
            if (idx == val)
            {
                offset++;   // skip index byte
                data.insert(data.begin(), &buff[offset], &buff[offset + PCIE_ENTRY_SIZE]);
                return data;
            }

            offset += PCIE_BLOCK_SIZE;
            num++;
        }
    }

    std::cerr << "getPcieInfo failed to read from EEPROM" << std::endl;
    return data;
}

bool setPcieInfo(const std::vector<uint8_t>& data)
{
    EepromArea peic = eepromMap[AreaID::PCIE];

    if (data.size() > peic.size)
    {
        std::cerr << "setPcieInfo exceeds max size" << std::endl;
        return false;
    }

    char buff[peic.size] = {0};
    std::copy(data.begin(), data.end(), buff);
    return writeEeprom(peic, buff);
}

std::vector<uint8_t> getNicInfo(const uint8_t& val)
{
    EepromArea nic = eepromMap[AreaID::NIC];
    char buff[nic.size + 1] = {0}; //Include checksum byte
    std::vector<uint8_t> data;

    if (readEeprom(nic, buff))
    {
        int offset = val * NIC_ENTRY_SIZE;
        data.insert(data.begin(), &buff[offset], &buff[offset + NIC_ENTRY_SIZE]);
        return data;
    }

    std::cerr << "getNicInfo failed to read from EEPROM" << std::endl;
    return data;
}

bool setNicInfo(const std::vector<uint8_t>& data)
{
    EepromArea nic = eepromMap[AreaID::NIC];

    if (data.size() > nic.size + 3)
    {
        std::cerr << "setNicInfo exceeds max size" << std::endl;
        return false;
    }

    int total = static_cast<int>(data[OFFSET_NIC_NUM]);

    if (total > MAX_NIC_NUM)
    {
        std::cerr << "setNicInfo exceeds max num" << std::endl;
        return false;
    }

    char buff[nic.size + 1] = {0}; //Include checksum byte

    if (readEeprom(nic, buff, true))
    {
        int offset = OFFSET_NIC_INDEX;
        int idx = 0;

        for (int i = 0; i < total; i++)
        {
            idx = static_cast<int>(data[offset]);

            if (idx >= MAX_NIC_NUM)
            {
                offset += NIC_BLOCK_SIZE;
                std::cerr << "setNicInfo idx exceeds max" << std::endl;
                continue;
            }

            offset++;  //skip index byte
            auto itStart = data.begin();
            std::advance(itStart, offset);
            std::copy(itStart, itStart + NIC_ENTRY_SIZE, &buff[idx * NIC_ENTRY_SIZE]);
            offset += NIC_ENTRY_SIZE;
        }

        return writeEeprom(nic, buff);
    }

    std::cerr << "setNicInfo failed to read from EEPROM" << std::endl;
    return false;
}

int main(int, char*[])
{
    // setup connection to dbus
    boost::asio::io_service io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // EepromDevice
    conn->request_name(eepromObject);
    auto server = sdbusplus::asio::object_server(conn);

    // Add MsEepromManager Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> ifaceMsEeprom =
        server.add_interface(eepromPath, eepromMsIntf);

    // Get BIOS config
    ifaceMsEeprom->register_method(
        "GetBiosConfig", []() {
            return getBiosConfig();
        });

    // set BIOS config
    ifaceMsEeprom->register_method(
        "SetBiosConfig", [](const uint8_t& val) {
            return setBiosConfig(val);
        });

    // set BIOS config info
    ifaceMsEeprom->register_method(
        "SetBiosConfigInfo", [](const std::vector<uint8_t>& data) {
            return setBiosConfigInfo(data);
        });

    // get CPU info
    ifaceMsEeprom->register_method(
        "GetCpuInfo", [](const uint8_t& val) {
            return getCPUInfo(val);
        });

    // set CPU config info
    ifaceMsEeprom->register_method(
        "SetCpuInfo", [](const std::vector<uint8_t>& data) {
            return setCPUInfo(data);
        });

    // get DIMM info
    ifaceMsEeprom->register_method(
        "GetMemInfo", [](const uint8_t& val) {
            return getMemInfo(val);
        });

    // set DIMM config info
    ifaceMsEeprom->register_method(
        "SetMemInfo", [](const std::vector<uint8_t>& data) {
            return setMemInfo(data);
        });

    // get PCIE info
    ifaceMsEeprom->register_method(
        "GetPcieInfo", [](const uint8_t& val) {
            return getPcieInfo(val);
        });

    // set PCIE config info
    ifaceMsEeprom->register_method(
        "SetPcieInfo", [](const std::vector<uint8_t>& data) {
            return setPcieInfo(data);
        });

    // get NIC info
    ifaceMsEeprom->register_method(
        "GetNicInfo", [](const uint8_t& val) {
            return getNicInfo(val);
        });

    // set NIC config info
    ifaceMsEeprom->register_method(
        "SetNicInfo", [](const std::vector<uint8_t>& data) {
            return setNicInfo(data);
        });

    ifaceMsEeprom->initialize();

    io.run();

    return 0;
}
