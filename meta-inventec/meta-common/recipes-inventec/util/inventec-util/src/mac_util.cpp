#include <iostream>
#include <fstream>
#include <netinet/ether.h>
#include <bits/stdc++.h>
#include <random>
#include "mac_util.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

using namespace std;

static const bool SEL_ASSERTED = true;
static const bool SEL_DEASSERTED = false;
static const uint8_t BMC_HEALTH_MASK_EMPTY_INVALID_FRU = static_cast<uint8_t>(0xA4);
static const uint8_t BMC_HEALTH_MASK_NO_MACADDRESS_PROGRAMMED = static_cast<uint8_t>(0xAC);

static void doMSBmcHealthSEL(std::vector<uint8_t>& eventData, bool isAsserted)
{
	static const uint16_t genId = 0x20;
	static constexpr char const* ipmiSELService = "xyz.openbmc_project.Logging.IPMI";
	static constexpr char const* ipmiSELPath = "/xyz/openbmc_project/Logging/IPMI";
	static constexpr char const* ipmiSELAddInterface = "xyz.openbmc_project.Logging.IPMI";
	static const std::string DBUS_OBJPATH_BMC_HEALTH = 	"/xyz/openbmc_project/sensors/oem_event_70h/oem_e0h/BMC_health";
	
	try{
		auto bus = sdbusplus::bus::new_default();

		sdbusplus::message::message writeSEL = bus.new_method_call(
			ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAdd");
		writeSEL.append(std::string("BMC Health"),
						DBUS_OBJPATH_BMC_HEALTH,
						eventData, isAsserted, genId);
		bus.call(writeSEL);
	}catch(const std::exception& e){
		fprintf(stderr, "%s:%d failed MS SEL e.what()=%s\n", __func__, __LINE__, e.what());
	}
}

static void printHelp(void)
{
	printf("mac_util usage:\n");
	printf("\n");
	printf("mac_util r <interface name>\n");
	printf("mac_util w <interface name> <mac address>\n");
	printf("\n");
	printf("example:\n");
	printf("mac_util r eth0\n");
	printf("0x38 0x68 0xdd 0x3e 0x99 0xec\n");
	printf("\n");
	printf("mac_util w eth0 02:00:ff:00:00:01\n");
	printf("\n");
	return;
}

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

int parseMAC(char *out, char *in)
{
	int i;
	int byte[MAC_UTIL_MAC_LEN] = {0};

	if (std::sscanf(in,
					"%02x:%02x:%02x:%02x:%02x:%02x",
					&byte[0], &byte[1], &byte[2],
					&byte[3], &byte[4], &byte[5]) != MAC_UTIL_MAC_LEN)
	{
		std::fprintf(stderr, "%s is an invalid MAC address\n", in);
		return -1;
	}

	for (i = 0; i < MAC_UTIL_MAC_LEN; i++)
	{
		out[i] = byte[i];
	}

	return 0;
}

bool setMAC(const string &path, char *mac_addr, const int &offset, const int &length)
{
	fstream file(path.c_str(), ios::in | ios::out | ios::binary | ios::ate);
	if (file.is_open())
	{
		file.seekp(offset, ios::beg);
		file.write(mac_addr, length);
		file.close();
		return true;
	}
	std::cerr << "Unable to open file " << path << std::endl;
	std::vector<uint8_t> eventData = {0xFF, 0xFF, 0xFF};
	eventData.at(0) = BMC_HEALTH_MASK_EMPTY_INVALID_FRU;
	eventData.at(1) = 0xFF; 
	eventData.at(2) = 0xFF;
	doMSBmcHealthSEL(eventData, SEL_ASSERTED);
	return false;
}

bool setMAC(const string &path, char *mac_addr, const int &offset)
{
	return setMAC(path, mac_addr, offset, MAC_UTIL_MAC_LEN);
}

bool getMAC(const string &path, const int &offset, std::vector<uint8_t> &mac_addr)
{
	char buff[MAC_UTIL_MAC_LEN] = {0};

	ifstream file(path.c_str(), ios::in | ios::binary | ios::ate);
	if (file.is_open())
	{
		file.seekg(offset, ios::beg);
		file.read(buff, MAC_UTIL_MAC_LEN);
		file.close();

		if (!mac_addr.empty())
		{
			mac_addr.clear();
		}
		std::copy(std::begin(buff), std::end(buff), std::back_inserter(mac_addr));
		return true;
	}
	std::cerr << "Unable to oepn file " << path << std::endl;
	std::vector<uint8_t> eventData = {0xFF, 0xFF, 0xFF};
	eventData.at(0) = BMC_HEALTH_MASK_EMPTY_INVALID_FRU;
	eventData.at(1) = 0xFF; 
	eventData.at(2) = 0xFF;
	doMSBmcHealthSEL(eventData, SEL_ASSERTED);
	return false;
}

bool getMAC(const string &path, const int &offset)
{
	bool ret = false;
	std::vector<uint8_t> mac_addr{};
	ret = getMAC(path, offset, mac_addr);
	if (ret)
	{
		std::fprintf(stdout, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
					 mac_addr[0], mac_addr[1], mac_addr[2],
					 mac_addr[3], mac_addr[4], mac_addr[5]);
	}
	return ret;
}

uint8_t getChecksumByte(const string &path, const int &offset)
{
	ifstream file(path.c_str(), ios::in | ios::binary | ios::ate);
	if (file.is_open())
	{
		char buff;
		file.seekg(offset, ios::beg);
		file.read(&buff, 1);
		file.close();
		return static_cast<uint8_t>(buff);
	}

	std::cerr << "Unable to oepn file " << path << std::endl;
	std::vector<uint8_t> eventData = {0xFF, 0xFF, 0xFF};
	eventData.at(0) = BMC_HEALTH_MASK_EMPTY_INVALID_FRU;
	eventData.at(1) = 0xFF; 
	eventData.at(2) = 0xFF;
	doMSBmcHealthSEL(eventData, SEL_ASSERTED);
	return 0;
}

bool setChecksumByte(const string &path, const uint8_t &checksum, const int &offset)
{
	fstream file(path.c_str(), ios::in | ios::out | ios::binary | ios::ate);
	if (file.is_open())
	{
		char buff = static_cast<char>(checksum);
		fprintf(stderr, "checksum=%02X buff=%02X \n", checksum, buff);
		file.seekp(offset, ios::beg);
		file.write(&buff, 1);
		file.close();
		return true;
	}

	std::cerr << "Unable to open file " << path << std::endl;
	std::vector<uint8_t> eventData = {0xFF, 0xFF, 0xFF};
	eventData.at(0) = BMC_HEALTH_MASK_EMPTY_INVALID_FRU;
	eventData.at(1) = 0xFF; 
	eventData.at(2) = 0xFF;
	doMSBmcHealthSEL(eventData, SEL_ASSERTED);
	return false;
}

uint8_t calculateChecksum(string &path, int &offset)
{
	// Get MAC addresses and the last offset
	size_t count;
	std::vector<uint8_t> fruData;
	count = sizeof(intfInfoList) / sizeof(IntfInfo);
	std::vector<uint8_t> mac_addresses{};
	for (int i = 0; i < count; i++)
	{
		std::vector<uint8_t> mac_addr{};
		offset = intfInfoList[i].offset;
		path = getEepromPath(intfInfoList[i].bus, intfInfoList[i].address);
		if (getMAC(path, intfInfoList[i].offset, mac_addr))
		{
			fruData.insert(fruData.end(), mac_addr.begin(), mac_addr.end());
		}
	}

	// Calculate new checksum
	if (!fruData.empty())
	{
		return calculateChecksum(fruData);
	}
	return 0xff;
}

bool recoverMAC(void)
{
	// Set Microsoft MAC address
	size_t count;
	count = sizeof(intfInfoList) / sizeof(IntfInfo);
	for (int i = 0; i < count; i++)
	{
		int offset;
		string path;

		// Microsoft prefix
		uint8_t mac_addr[] = {0x00, 0x03, 0xFF, 0x00, 0x00, 0x00};

		// Set blade slot id
		mac_addr[MAC_UTIL_MAC_LEN - 1] = static_cast<uint8_t>(i);
		offset = intfInfoList[i].offset;
		path = getEepromPath(intfInfoList[i].bus, intfInfoList[i].address);
		if (!setMAC(path, reinterpret_cast<char *>(mac_addr), offset))
		{
			return false;
		}
	}

	// Calucluate new checksum & get the last one path and offset
	string path;
	int offset;
	uint8_t checksum;
	checksum = calculateChecksum(path, offset);

	// Set checksum byte
	if (checksum != 0xff)
	{
		if (!setChecksumByte(path, checksum, offset + MAC_UTIL_MAC_LEN))
		{
			// Set checksum byte fail
			return false;
		}
		return true;
	}
	return false;
}

int main(int argc, char *argv[])
{
	string path;
	int offset = 0;
	char mac_addr[MAC_UTIL_MAC_LEN] = {0};
	IntfInfo *intfInfo;
	uint8_t checksum, checksum_byte;
	std::vector<uint8_t> eventData = {0xFF, 0xFF, 0xFF}; 

	for (int i = 0; i < argc; i++)
	{
		switch (argv[i][0])
		{
		case 'r':
			if (argc < i + 2)
			{
				std::fprintf(stderr, "mac_util:input not enought\n");
				printHelp();
				return -1;
			}
			intfInfo = findIntfInfo(argv[i + 1]);
			if (intfInfo == NULL)
			{
				std::fprintf(stderr, "mac_util:interface %s not found\n", argv[i + 1]);
				return -1;
			}

			path = getEepromPath(intfInfo->bus, intfInfo->address);

			// Calculate checksum
			checksum = calculateChecksum(path, offset);
			if (checksum == 0xff)
			{
				// Calculate checksum fail
				std::cerr << "Calculate checksum fail!" << std::endl;
				return -1;
			}

			// Get checksum byte
			checksum_byte = getChecksumByte(path, offset + MAC_UTIL_MAC_LEN);

			// Verify checksum
			if ((checksum == 0) || (checksum != checksum_byte))
			{
				// Recover MAC if checksum fail
				if (!recoverMAC())
				{
					// Recover MAC fail
					std::cerr << "Recover MAC addresses fail!" << std::endl;
					eventData.at(0) = BMC_HEALTH_MASK_NO_MACADDRESS_PROGRAMMED;
					eventData.at(1) = 0xFF; 
					eventData.at(2) = 0xFF;
					doMSBmcHealthSEL(eventData, SEL_ASSERTED);
					return -1;
				}
			}

			if (!getMAC(path, intfInfo->offset))
			{
				// Get MAC fail
				std::cerr << "Get MAC fail!" << std::endl;
				return -1;
			}

			/*leave loop*/
			i = argc;
			break;
		case 'w':
			if (argc < i + 3)
			{
				std::fprintf(stderr, "mac_util:input not enought\n");
				printHelp();
				return -1;
			}
			intfInfo = findIntfInfo(argv[i + 1]);
			if (intfInfo == NULL)
			{
				std::fprintf(stderr, "mac_util:interface %s not found\n", argv[i + 1]);
				return -1;
			}

			path = getEepromPath(intfInfo->bus, intfInfo->address);

			if (!parseMAC(mac_addr, argv[i + 2]))
			{
				// Set MAC address
				if (!setMAC(path, mac_addr, intfInfo->offset))
				{
					// Set MAC fail
					std::cerr << "Set MAC fail!" << std::endl;	
					return -1;
				}

				// Calculate checksum
				checksum = calculateChecksum(path, offset);
				if (checksum == 0xff)
				{
					// Calculate checksum fail
					std::cerr << "Calculate checksum fail!" << std::endl;
					return -1;
				}

				// Set checksum byte
				if (!setChecksumByte(path, checksum, offset + MAC_UTIL_MAC_LEN))
				{
					// Set checksum byte fail
					std::cerr << "Set checksum byte fail!" << std::endl;
					return -1;
				}
			}
			/*leave loop*/
			i = argc;
			break;
		case 'h':
			/*leave loop*/
			printHelp();
			i = argc;
			break;

		case 't':
			if (argc < i + 3)
			{
				std::fprintf(stderr, "mac_util:input not enought\n");
				printHelp();
				return -1;
			}
			intfInfo = findIntfInfo(argv[i + 1]);
			if (intfInfo == NULL)
			{
				std::fprintf(stderr, "mac_util:interface %s not found\n", argv[i + 1]);
				return -1;
			}

			path = getEepromPath(intfInfo->bus, intfInfo->address);

			// Calculate checksum
			checksum = 0xFF;
			// Get checksum byte
			checksum_byte = getChecksumByte(path, offset + MAC_UTIL_MAC_LEN);
			fprintf(stderr, "checsum=%02X checksum_byte=%02X\n", checksum, checksum_byte);
			// Verify checksum
			if (checksum != checksum_byte)
			{
				eventData.at(0) = BMC_HEALTH_MASK_NO_MACADDRESS_PROGRAMMED;
				eventData.at(1) = 0xFF;
				eventData.at(2) = 0xFF;
				doMSBmcHealthSEL(eventData, SEL_ASSERTED);
				return -1;
			}
			/*leave loop*/
			i = argc;
			break;
		default:
			break;
		}
	}
	return 0;
}
