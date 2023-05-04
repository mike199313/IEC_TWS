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

#include "common_types.hpp"
#include "flow_control.hpp"
#include "knobs/knob.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "utility/devices_configuration.hpp"
#include "utility/file_matcher.hpp"
#include "utility/performance_monitor.hpp"
#include "utility/ranges.hpp"
#include "utility/types.hpp"

#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <iostream>
#include <sdbusplus/asio/object_server.hpp>
#include <unordered_map>

namespace nodemanager
{

static const uint8_t kHwmonPeciCpuBaseAddress = 0x30;
static const uint8_t kHwmonPeciPvcBaseAddress = 0x48;
static const uint8_t kHwmonI2cPsuBaseAddress = 0x58;
static const uint8_t kHwmonSmbusPvcBaseBus = 40;
static const std::chrono::seconds kHwmonDiscoveryPeriod{10};

static const unsigned kBusPathIndex = 1;
static const unsigned kAddressPathIndex = 2;
static const unsigned kHwmonNamePathIndex = 3;
static const unsigned kFileNamePathIndex = 4;

class HwmonFileProviderIf
{
  public:
    virtual ~HwmonFileProviderIf() = default;
    virtual std::filesystem::path getFile(SensorReadingType sensor,
                                          DeviceIndex idx) const = 0;
    virtual std::filesystem::path getFile(KnobType knob,
                                          DeviceIndex idx) const = 0;
};

static const boost::container::flat_map<std::pair<std::string, std::string>,
                                        SensorReadingType>
    kHwmonToSensorReadings = {
        {{"pvcpower", "power2_average"}, SensorReadingType::pciePower},
        {{"cpupower", "power1_average"}, SensorReadingType::cpuPackagePower},
        {{"cpupower", "power1_cap_max"},
         SensorReadingType::cpuPackagePowerCapabilitiesMax},
        {{"cpupower", "power1_cap_min"},
         SensorReadingType::cpuPackagePowerCapabilitiesMin},
        {{"cpupower", "power1_cap"}, SensorReadingType::cpuPackagePowerLimit},
        {{"cpupower", "energy1_input"}, SensorReadingType::cpuEnergy},
        {{"dimmpower", "power1_average"}, SensorReadingType::dramPower},
        {{"dimmpower", "power1_cap_max"},
         SensorReadingType::dramPackagePowerCapabilitiesMax},
        {{"dimmpower", "power1_cap"}, SensorReadingType::dramPowerLimit},
        {{"dimmpower", "energy1_input"}, SensorReadingType::dramEnergy},
        {{"platformpower", "power1_average"},
         SensorReadingType::dcPlatformPowerCpu},
        {{"platformpower", "power1_cap"},
         SensorReadingType::dcPlatformPowerLimit},
        {{"platformpower", "power1_cap_max"},
         SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu},
        {{"platformpower", "energy1_input"},
         SensorReadingType::dcPlatformEnergy},
        {{"psu", "power1_input"}, SensorReadingType::acPlatformPower},
        {{"psu", "power2_input"}, SensorReadingType::dcPlatformPowerPsu},
        {{"psu", "power1_rated_max"},
         SensorReadingType::acPlatformPowerCapabilitiesMax},
        {{"psu", "power2_rated_max"},
         SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu},
};

static const boost::container::flat_map<std::pair<std::string, std::string>,
                                        KnobType>
    kHwmonToKnob = {
        {{"platformpower", "power1_cap"}, KnobType::DcPlatformPower},
        {{"cpupower", "power1_cap"}, KnobType::CpuPackagePower},
        {{"dimmpower", "power1_cap"}, KnobType::DramPower},
        {{"pvcpower", "power1_cap"}, KnobType::PciePower}};

static const boost::container::flat_map<std::string,
                                        std::tuple<uint8_t, DeviceIndex>>
    kDeviceAddressMap = {
        {"pvcpower", std::make_tuple(kHwmonPeciPvcBaseAddress, kMaxPcieNumber)},
        {"platformpower",
         std::make_tuple(kHwmonPeciCpuBaseAddress, kMaxPlatformNumber)},
        {"cpupower", std::make_tuple(kHwmonPeciCpuBaseAddress, kMaxCpuNumber)},
        {"dimmpower", std::make_tuple(kHwmonPeciCpuBaseAddress, kMaxCpuNumber)},
        {"psu", std::make_tuple(kHwmonI2cPsuBaseAddress, kMaxPsuNumber)},
};

/**
 * @brief This class periodically scans folder with hwmon files and
 *
 */
class HwmonFileProvider : public HwmonFileProviderIf
{
  public:
    HwmonFileProvider(std::shared_ptr<sdbusplus::asio::connection> busArg,
                      std::filesystem::path rootPathArg = "/sys/bus",
                      bool startDiscoveryTimer = true) :
        hwmonDiscoveryTimer(busArg->get_io_context()),
        hwmonDiscoveryTimeout(std::chrono::seconds(1))
    {
        installConfigs(rootPathArg);
        if (startDiscoveryTimer)
        {
            runFileDiscovery();
        }
    }

    virtual ~HwmonFileProvider() = default;

    std::filesystem::path getFile(SensorReadingType sensor,
                                  DeviceIndex idx) const final
    {
        const auto& it = sensorsToHwmonMap.find(std::make_pair(sensor, idx));
        if (it != sensorsToHwmonMap.cend())
        {
            return it->second;
        }
        return std::filesystem::path{};
    }

    std::filesystem::path getFile(KnobType knob, DeviceIndex idx) const final
    {
        const auto& it = knobsToHwmonMap.find(std::make_pair(knob, idx));
        if (it != knobsToHwmonMap.cend())
        {
            return it->second;
        }
        return std::filesystem::path{};
    }

    void discoverFiles()
    {
        auto perf = Perf("Hwmon-discoveryFiles-duration",
                         std::chrono::milliseconds{20});
        discoverHwmonFiles();
    }

  protected:
    using FileMatcherResults =
        std::tuple<uint32_t, uint32_t, std::string, std::string>;

    using DiscoveredPaths =
        FileMatcher<FileMatcherResults>::PathDecompositionMap;

    std::optional<std::future<DiscoveredPaths>> future;

  private:
    template <class T>
    using ElementToPathMap =
        boost::container::flat_map<std::pair<T, DeviceIndex>,
                                   std::filesystem::path>;

    void installConfigs(std::filesystem::path rootPath)
    {
        fileMatchers.emplace_back(
            rootPath / "peci/devices",
            std::regex("peci-0/([0-9]+)-([0-9a-fA-F]+)/"
                       "peci-(.+)\\.[0-9]+/hwmon/hwmon[0-9]+/"
                       "(power1_average|power1_cap_min|power1_cap_max"
                       "|power1_cap|energy1_input)$"),
            [](const std::smatch& match) {
                return FileMatcherResults{
                    std::stoul(match[kBusPathIndex]),
                    std::stoul(match[kAddressPathIndex], nullptr, 16),
                    match[kHwmonNamePathIndex], match[kFileNamePathIndex]};
            });

        fileMatchers.emplace_back(
            rootPath / "i2c/devices",
            std::regex("i2c-[0-9]+/([0-9]+)-00(48|4a|4c)/peci-[0-9]+()/"
                       "[0-9]+-30/hwmon/hwmon[0-9]+/"
                       "(power2_average|power1_cap)$"),
            [](const std::smatch& match) {
                return FileMatcherResults{
                    std::stoul(match[kBusPathIndex]),
                    std::stoul(match[kAddressPathIndex], nullptr, 16),
                    "pvcpower", match[kFileNamePathIndex]};
            });

        fileMatchers.emplace_back(
            rootPath / "i2c/devices",
            std::regex(
                "i2c-[0-9]+/([0-9]+)-00([0-9a-fA-F]+)/hwmon/hwmon[0-9]+()/"
                "(power1_input|power1_rated_max|power2_input|power2_rated_max)"
                "$"),
            [](const std::smatch& match) {
                return FileMatcherResults{
                    std::stoul(match[kBusPathIndex]),
                    std::stoul(match[kAddressPathIndex], nullptr, 16), "psu",
                    match[kFileNamePathIndex]};
            });
    }

    void discoverHwmonFiles()
    {
        Logger::log<LogLevel::debug>(
            "[HwmonFileProvider]: discovering hwmon files");

        if (future)
        {
            if (future->valid() && future->wait_for(std::chrono::seconds(0)) ==
                                       std::future_status::ready)
            {
                const DiscoveredPaths& discoveredPaths = future->get();
                addFileMapping(kHwmonToSensorReadings, discoveredPaths,
                               sensorsToHwmonMap);
                addFileMapping(kHwmonToKnob, discoveredPaths, knobsToHwmonMap);

                removeNonexistentMapping(discoveredPaths, sensorsToHwmonMap);
                removeNonexistentMapping(discoveredPaths, knobsToHwmonMap);
            }
        }
        if (!future || !future->valid())
        {
            future = std::move(
                std::async(std::launch::async,
                           [matchers = fileMatchers]() -> DiscoveredPaths {
                               DiscoveredPaths discoveredPaths;
                               for (const auto& matcher : matchers)
                               {
                                   matcher.findFiles(discoveredPaths, 1);
                               }
                               return discoveredPaths;
                           }));
        }
    }

    template <class T>
    void addFileMapping(
        const boost::container::flat_map<std::pair<std::string, std::string>,
                                         T>& hwmonToSensorReadings,
        const DiscoveredPaths& newPaths, ElementToPathMap<T>& output)
    {
        for (const auto& [hwmonPath, pathDetails] : newPaths)
        {
            Logger::log<LogLevel::debug>(
                "[HwmonFileProvider]: processing file: %s", hwmonPath);

            auto [bus, address, hwmonDeviceType, fileName] = pathDetails;

            const auto& it = hwmonToSensorReadings.find(
                std::make_pair(hwmonDeviceType, fileName));
            if (it != hwmonToSensorReadings.end())
            {
                if (auto idx = getDeviceIndex(hwmonDeviceType, bus, address))
                {
                    auto key = std::make_pair(it->second, *idx);
                    if (output[key] != hwmonPath)
                    {
                        output[key] = hwmonPath;
                        Logger::log<LogLevel::info>(
                            "[HwmonFileProvider]: new hwmon file detected : "
                            "%s",
                            hwmonPath);
                    }
                }
                else
                {
                    Logger::log<LogLevel::debug>(
                        "[HwmonFileProvider]: Cannot calculate DeviceIndex for "
                        "address: %d "
                        "of device type: %s",
                        address, hwmonDeviceType);
                }
            }
            else
            {
                Logger::log<LogLevel::debug>(
                    "[HwmonFileProvider]: %s no mapping found for type: %s, "
                    "filename: %s",
                    hwmonPath, hwmonDeviceType, fileName);
            }
        }
    }

    template <class T>
    void removeNonexistentMapping(const DiscoveredPaths& newPaths,
                                  ElementToPathMap<T>& output)
    {
        std::vector<std::pair<T, DeviceIndex>> toRemove;
        for (const auto& [key, path] : output)
        {
            if (!newPaths.contains(path))
            {
                Logger::log<LogLevel::info>(
                    "[HwmonFileProvider]: hwmon file %s doesn't exits, "
                    "removing its mapping",
                    path);
                toRemove.push_back(key);
            }
        }
        for (const auto& key : toRemove)
        {
            output.erase(key);
        }
    }

    std::optional<DeviceIndex> getDeviceIndex(std::string deviceType,
                                              uint32_t bus, uint32_t address)
    {
        const auto& it = kDeviceAddressMap.find(deviceType);
        if (it != kDeviceAddressMap.end())
        {
            auto [baseAddress, maxIdx] = it->second;
            DeviceIndex idx{maxIdx};

            if ((address >= baseAddress) && ((address - baseAddress) < maxIdx))
            {
                if (deviceType == "pvcpower")
                {
                    /* Each of PVC devices is installed on specified bus:
                       bus 40 -> index 0, bus 41 -> index 1, bus 42 -> index 2
                       bus 80 -> index 3, bus 81 -> index 4, bus 82 -> index 5
                     */

                    idx = safeCast<DeviceIndex>(
                        3 * (bus / kHwmonSmbusPvcBaseBus - 1) +
                            bus % kHwmonSmbusPvcBaseBus,
                        kAllDevices);
                }
                else
                {
                    idx = safeCast<DeviceIndex>(address - baseAddress,
                                                kAllDevices);
                }
            }
            if (idx < maxIdx)
            {
                return idx;
            }
        }
        Logger::log<LogLevel::debug>(
            "[HwmonFileProvider]: Cannot calculate DeviceIndex for "
            "bus: %d, address: %d of device type: %s",
            bus, address, deviceType);
        return std::nullopt;
    }

    void runFileDiscovery()
    {
        discoverFiles();
        hwmonDiscoveryTimer.expires_after(hwmonDiscoveryTimeout);
        hwmonDiscoveryTimer.async_wait([this](boost::system::error_code ec) {
            if (ec)
            {
                Logger::log<LogLevel::warning>(
                    "[HwmonFileProvider]: Unexpected hwmon discovery timer"
                    "cancellation. You should probably restart NodeManager"
                    "service. Error code: %d",
                    ec);
                return;
            }
            hwmonDiscoveryTimeout = kHwmonDiscoveryPeriod;
            runFileDiscovery();
        });
    }

    boost::asio::steady_timer hwmonDiscoveryTimer;
    std::chrono::seconds hwmonDiscoveryTimeout;
    ElementToPathMap<SensorReadingType> sensorsToHwmonMap;
    ElementToPathMap<KnobType> knobsToHwmonMap;
    std::vector<FileMatcher<FileMatcherResults>> fileMatchers;
};
} // namespace nodemanager
