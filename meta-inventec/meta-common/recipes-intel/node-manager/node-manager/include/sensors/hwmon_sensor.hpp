/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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
#include "devices_manager/hwmon_file_provider.hpp"
#include "loggers/log.hpp"
#include "sensor.hpp"
#include "utility/devices_configuration.hpp"
#include "utility/ranges.hpp"
#include "utility/units.hpp"

#include <filesystem>
#include <fstream>

namespace nodemanager
{

static constexpr const auto kHwmonReadRetriesCount = 2;

/**
 * @brief HwmonSensor class used as a base for all hwmon related sensors to
 * provide readings from them.
 *
 */
class HwmonSensor : public Sensor
{
  public:
    HwmonSensor() = delete;
    HwmonSensor(const HwmonSensor&) = delete;
    HwmonSensor& operator=(const HwmonSensor&) = delete;
    HwmonSensor(HwmonSensor&&) = delete;
    HwmonSensor& operator=(HwmonSensor&&) = delete;

    HwmonSensor(
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
        std::shared_ptr<HwmonFileProviderIf> hwmonProviderArg) :
        Sensor(sensorReadingsManagerArg),
        hwmonProvider(hwmonProviderArg)
    {
        installSensorReadings();
    }

    void installSensorReadings()
    {
        constexpr std::array pcieTypes = {SensorReadingType::pciePower};
        for (SensorReadingType type : pcieTypes)
        {
            for (DeviceIndex pcieIndex = 0; pcieIndex < kMaxPcieNumber;
                 ++pcieIndex)
            {
                readings.emplace_back(
                    sensorReadingsManager->createSensorReading(type,
                                                               pcieIndex));
            }
        }

        constexpr std::array cpuTypes = {
            SensorReadingType::cpuPackagePower,
            SensorReadingType::cpuPackagePowerCapabilitiesMax,
            SensorReadingType::cpuPackagePowerCapabilitiesMin,
            SensorReadingType::cpuPackagePowerLimit,
            SensorReadingType::cpuEnergy,
            SensorReadingType::dramPower,
            SensorReadingType::dramPackagePowerCapabilitiesMax,
            SensorReadingType::dramPowerLimit,
            SensorReadingType::dramEnergy};
        for (SensorReadingType type : cpuTypes)
        {
            for (DeviceIndex cpuIndex = 0; cpuIndex < kMaxCpuNumber; ++cpuIndex)
            {
                readings.emplace_back(
                    sensorReadingsManager->createSensorReading(type, cpuIndex));
            }
        }

        constexpr std::array platformTypes = {
            SensorReadingType::dcPlatformPowerCpu,
            SensorReadingType::dcPlatformPowerLimit,
            SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu,
            SensorReadingType::dcPlatformEnergy};
        for (SensorReadingType type : platformTypes)
        {
            for (DeviceIndex platformIndex = 0;
                 platformIndex < kMaxPlatformNumber; ++platformIndex)
            {
                readings.emplace_back(
                    sensorReadingsManager->createSensorReading(type,
                                                               platformIndex));
            }
        }

        constexpr std::array psuTypes = {
            SensorReadingType::acPlatformPower,
            SensorReadingType::acPlatformPowerCapabilitiesMax,
            SensorReadingType::dcPlatformPowerPsu,
            SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu};
        for (SensorReadingType type : psuTypes)
        {
            for (DeviceIndex psuIndex = 0; psuIndex < kMaxPsuNumber; ++psuIndex)
            {
                readings.emplace_back(
                    sensorReadingsManager->createSensorReading(type, psuIndex));
            }
        }
    }

    virtual ~HwmonSensor() = default;

    void reportStatus(nlohmann::json& out) const override
    {
        for (const auto& sensorReading : readings)
        {
            nlohmann::json tmp;
            std::filesystem::path filePath =
                hwmonProvider->getFile(sensorReading->getSensorReadingType(),
                                       sensorReading->getDeviceIndex());
            auto type = enumToStr(kSensorReadingTypeNames,
                                  sensorReading->getSensorReadingType());
            tmp["Status"] =
                enumToStr(sensorReadingStatusNames, sensorReading->getStatus());
            tmp["Health"] = enumToStr(healthNames, sensorReading->getHealth());
            tmp["DeviceIndex"] = sensorReading->getDeviceIndex();
            std::visit([&tmp](auto&& value) { tmp["Value"] = value; },
                       sensorReading->getValue());
            tmp["HwmonPath"] = filePath;
            out["Sensors-hwmon"][type].push_back(tmp);
        }
    }

    static double convertHwmonUnitsToNmUnits(double value)
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::ratio<1>>>(
                   std::chrono::duration<double, std::ratio<1, 1000>>{value})
            .count();
    }

    static double convertHwmonUnitsToNmUnitsPsu(double value)
    {
        return std::chrono::duration_cast<
                   std::chrono::duration<double, std::ratio<1>>>(
                   std::chrono::duration<double, std::ratio<1, 1000000>>{value})
            .count();
    }

    void run() override final
    {
        auto perf =
            Perf("SensorSet-Hwmon-run-duration", std::chrono::milliseconds{10});
        for (const auto& sensorReading : readings)
        {
            SensorReadingType type = sensorReading->getSensorReadingType();
            DeviceIndex index = sensorReading->getDeviceIndex();
            std::pair<SensorReadingType, DeviceIndex> typeAndIndex = {type,
                                                                      index};
            auto future = futures.find(typeAndIndex);
            if (future != futures.end())
            {
                if (future->second.valid() &&
                    future->second.wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                {
                    auto [value, status] = future->second.get();

                    if (status == SensorReadingStatus::valid)
                    {
                        retries[typeAndIndex] = 0;
                        sensorReading->setStatus(status);
                        sensorReading->updateValue(value);
                    }
                    else if (retries[typeAndIndex] < kHwmonReadRetriesCount)
                    {
                        retries[typeAndIndex]++;
                    }
                    else
                    {
                        sensorReading->setStatus(status);
                    }
                }
                else
                {
                    sensorReading->setStatus(SensorReadingStatus::unavailable);
                }
            }

            if (future == futures.end() || future->second.valid() == false)
            {
                if (!isEndpointAvailable(type, index))
                {
                    sensorReading->setStatus(SensorReadingStatus::unavailable);
                    continue;
                }
                auto filePath = hwmonProvider->getFile(type, index);
                futures.insert_or_assign(
                    typeAndIndex,
                    std::async(std::launch::async, [filePath, type]() {
                        std::ifstream hwmonFile(filePath);
                        if (!hwmonFile.good())
                        {
                            return std::make_pair(
                                double{0}, SensorReadingStatus::unavailable);
                        }

                        double valueFromHwmonFile = 0;
                        double value = 0;

                        hwmonFile >> valueFromHwmonFile;
                        if (hwmonFile.good())
                        {
                            if (type == SensorReadingType::acPlatformPower ||
                                type == SensorReadingType::
                                            acPlatformPowerCapabilitiesMax ||
                                type == SensorReadingType::dcPlatformPowerPsu ||
                                type == SensorReadingType::
                                            dcPlatformPowerCapabilitiesMaxPsu)
                            {
                                value =
                                    HwmonSensor::convertHwmonUnitsToNmUnitsPsu(
                                        valueFromHwmonFile);
                            }
                            else
                            {
                                value = HwmonSensor::convertHwmonUnitsToNmUnits(
                                    valueFromHwmonFile);
                            }
                            return std::make_pair(value,
                                                  SensorReadingStatus::valid);
                        }
                        else
                        {
                            return std::make_pair(double{0},
                                                  SensorReadingStatus::invalid);
                        }
                    }));
            }
        }
    }

  protected:
    std::shared_ptr<HwmonFileProviderIf> hwmonProvider;

    std::map<std::pair<SensorReadingType, DeviceIndex>,
             std::future<std::pair<double, SensorReadingStatus>>>
        futures;
    std::map<std::pair<SensorReadingType, DeviceIndex>, uint8_t> retries;

  private:
    bool isEndpointAvailable(const SensorReadingType type,
                             const DeviceIndex index) const
    {
        auto filePath = hwmonProvider->getFile(type, index);
        if (filePath.empty())
        {
            return false;
        }
        switch (type)
        {
            case SensorReadingType::cpuPackagePower:
                return sensorReadingsManager->isPowerStateOn();
            case SensorReadingType::acPlatformPower:
            case SensorReadingType::acPlatformPowerCapabilitiesMax:
            case SensorReadingType::dcPlatformPowerPsu:
            case SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu:
                return true;
            case SensorReadingType::pciePower:
                return sensorReadingsManager->isGpuPowerStateOn();
            default:
                return sensorReadingsManager->isPowerStateOn() &&
                       sensorReadingsManager->isCpuAvailable(index);
        }
    }
};

} // namespace nodemanager