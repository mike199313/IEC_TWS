/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with
 * no express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#pragma once

#include "utils/conversion.hpp"

#include <filesystem>
#include <fstream>
#include <optional>

using namespace nodemanager;

enum class HwmonGroup
{
    cpu,
    dimm,
    platform,
    pvc,
    psu
};

enum class HwmonFileType
{
    min,
    max,
    current,
    limit,
    energy,
    pciePower,
    psuAcPower,
    psuDcPower,
    psuAcPowerMax,
    psuDcPowerMax,
};

/**
 * @brief This class privdes helpers to play with hwmon files/directories for
 * CPU
 */
class HwmonFileManager
{
  public:
    HwmonFileManager() :
        rootPath(std::filesystem::temp_directory_path() / "nm-hwmon-ut")
    {
    }
    std::filesystem::path createCpuFile(uint32_t address, HwmonGroup groupname,
                                        std::optional<HwmonFileType> fileType,
                                        unsigned int index = 3,
                                        unsigned int value = 0)
    {
        std::string filename = getFilename(fileType);
        auto filePath =
            rootPath / "peci/devices" / "peci-0" /
            (std::to_string(index) + "-" + numberToHexString(address)) /
            ("peci-" + groupnameToDir(groupname) + ".0") / "hwmon/hwmon65";
        std::filesystem::create_directories(filePath);
        std::ofstream(filePath / filename) << value << "\n";
        return filePath / filename;
    }

    std::filesystem::path createPvcFile(uint32_t bus, uint32_t address,
                                        HwmonGroup groupname,
                                        std::optional<HwmonFileType> fileType,
                                        unsigned int index = 3,
                                        unsigned int value = 0)
    {
        std::string fileName = getFilename(fileType);
        auto filePath =
            rootPath / "i2c/devices" / ("i2c-" + std::to_string(bus)) /
            (std::to_string(bus) + "-00" + numberToHexString(address)) /
            ("peci-" + std::to_string(index)) /
            (std::to_string(index) + "-30") / "hwmon/hwmon65";
        std::filesystem::create_directories(filePath);
        std::ofstream(filePath / fileName) << value << "\n";
        return filePath / fileName;
    }

    std::filesystem::path createPsuFile(uint32_t bus, uint32_t address,
                                        HwmonGroup groupname,
                                        std::optional<HwmonFileType> fileType,
                                        std::string value = "0")
    {
        std::string fileName = getFilename(fileType);
        auto filePath =
            rootPath / "i2c/devices" / ("i2c-" + std::to_string(bus)) /
            (std::to_string(bus) + "-00" + numberToHexString(address)) /
            "hwmon/hwmon65";
        std::filesystem::create_directories(filePath);
        std::ofstream(filePath / fileName) << value << "\n";
        return filePath / fileName;
    }

    virtual ~HwmonFileManager()
    {
        removeHwmonDirectories();
    };

    void removeHwmonDirectories()
    {
        std::filesystem::remove_all(rootPath);
    }

    std::filesystem::path getRootPath()
    {
        return rootPath;
    }

    void writeFile(std::filesystem::path filePath, unsigned int value)
    {
        std::ofstream(filePath, std::ios_base::out | std::ios_base::trunc)
            << value << "\n";
    }

    std::string readFile(std::filesystem::path filePath)
    {
        std::string value;
        std::ifstream hwmonFile(filePath);
        hwmonFile >> value;
        return value;
    }

    std::string groupnameToDir(HwmonGroup group)
    {
        switch (group)
        {
            case HwmonGroup::cpu:
                return std::string("cpupower");
            case HwmonGroup::dimm:
                return std::string("dimmpower");
            case HwmonGroup::platform:
                return std::string("platformpower");
            case HwmonGroup::pvc:
                return std::string("pvcpower");
            case HwmonGroup::psu:
                return std::string("psu");
        }

        throw std::invalid_argument(
            "Add a new group to the supported groups list");
    }

    std::string filetypeToFilename(HwmonFileType type)
    {
        switch (type)
        {
            case HwmonFileType::min:
                return "power1_cap_min";
            case HwmonFileType::max:
                return "power1_cap_max";
            case HwmonFileType::limit:
                return "power1_cap";
            case HwmonFileType::current:
                return "power1_average";
            case HwmonFileType::energy:
                return "energy1_input";
            case HwmonFileType::pciePower:
                return "power2_average";
            case HwmonFileType::psuAcPower:
                return "power1_input";
            case HwmonFileType::psuDcPower:
                return "power2_input";
            case HwmonFileType::psuAcPowerMax:
                return "power1_rated_max";
            case HwmonFileType::psuDcPowerMax:
                return "power2_rated_max";
        }

        throw std::invalid_argument(
            "Add a new filetype to the supported types list");
    }

  private:
    std::filesystem::path rootPath;

    std::string getFilename(std::optional<HwmonFileType> fileType)
    {
        if (std::nullopt != fileType)
        {
            return filetypeToFilename(*fileType);
        }
        else
        {
            return "invalid";
        }
    }
};