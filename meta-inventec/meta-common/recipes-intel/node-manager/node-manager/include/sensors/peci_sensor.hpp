/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2022 Intel Corporation.
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

#include "clock.hpp"
#include "peci/peci_commands.hpp"
#include "peci/peci_types.hpp"
#include "sensor.hpp"
#include "sensor_readings_manager.hpp"

#include <future>

namespace nodemanager
{
class PeciSensor : public Sensor
{
  public:
    PeciSensor() = delete;
    PeciSensor(const PeciSensor&) = delete;
    PeciSensor& operator=(const PeciSensor&) = delete;
    PeciSensor(PeciSensor&&) = delete;
    PeciSensor& operator=(PeciSensor&&) = delete;

    PeciSensor(
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
        std::shared_ptr<PeciCommandsIf> peciCommandsArg,
        DeviceIndex maxCpuNumberArg) :
        Sensor(sensorReadingsManagerArg),
        maxCpuNumber(maxCpuNumberArg), peciCommands(peciCommandsArg)
    {
        installSensorReadings();
    }

    virtual ~PeciSensor() = default;

    void run() override final
    {
        auto perf =
            Perf("SensorSet-peci-run-duration", std::chrono::milliseconds{10});

        std::vector<std::optional<uint32_t>> cpuIds(maxCpuNumber, std::nullopt);
        for (DeviceIndex cpuIndex = 0; cpuIndex < maxCpuNumber; cpuIndex++)
        {
            if (auto sensorReadingCpuId =
                    sensorReadingsManager
                        ->getAvailableAndValueValidSensorReading(
                            SensorReadingType::cpuPackageId, cpuIndex))
            {
                if (std::holds_alternative<uint32_t>(
                        sensorReadingCpuId->getValue()))
                {
                    cpuIds[cpuIndex] =
                        std::get<uint32_t>(sensorReadingCpuId->getValue());
                }
            }
        }

        for (const auto& peciSensor : readings)
        {
            DeviceIndex deviceIndex = peciSensor->getDeviceIndex();
            if (!isEndpointAvailable(deviceIndex))
            {
                peciSensor->setStatus(SensorReadingStatus::unavailable);
                continue;
            }

            const auto key =
                std::make_pair(deviceIndex, peciSensor->getSensorReadingType());
            auto futureIt = futureSamples.find(key);
            if (futureIt != futureSamples.end())
            {
                if (futureIt->second.valid() &&
                    futureIt->second.wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                {
                    if (auto value = futureIt->second.get())
                    {
                        peciSensor->updateValue(*value);
                        peciSensor->setStatus(SensorReadingStatus::valid);
                    }
                    else
                    {
                        peciSensor->setStatus(SensorReadingStatus::invalid);
                    }
                }
                else
                {
                    peciSensor->setStatus(SensorReadingStatus::unavailable);
                }
            }

            if (futureIt == futureSamples.end() ||
                futureIt->second.valid() == false)
            {
                futureSamples.insert_or_assign(
                    key,
                    std::async(
                        std::launch::async,
                        [key, cpuIds,
                         peciIf = peciCommands]() -> std::optional<ValueType> {
                            auto& [idx, sensorType] = key;
                            switch (sensorType)
                            {
                                case SensorReadingType::cpuPackageId:
                                    return peciIf->getCpuId(idx);
                                case SensorReadingType::
                                    prochotRatioCapabilitiesMin:
                                case SensorReadingType::
                                    turboRatioCapabilitiesMin: {
                                    if (cpuIds[idx])
                                    {
                                        return peciIf->getMinOperatingRatio(
                                            idx, *cpuIds[idx]);
                                    }
                                    return {std::nullopt};
                                }
                                case SensorReadingType::
                                    prochotRatioCapabilitiesMax: {
                                    if (cpuIds[idx])
                                    {
                                        return peciIf->getMaxNonTurboRatio(
                                            idx, *cpuIds[idx]);
                                    }
                                    return {std::nullopt};
                                }
                                case SensorReadingType::
                                    turboRatioCapabilitiesMax: {
                                    if (cpuIds[idx])
                                    {
                                        return peciIf->detectMaxTurboRatio(
                                            idx, *cpuIds[idx]);
                                    }
                                    return {std::nullopt};
                                }
                                case SensorReadingType::cpuDieMask: {
                                    if (cpuIds[idx])
                                    {
                                        return peciIf->getCpuDieMask(idx);
                                    }
                                    return {std::nullopt};
                                }

                                default:
                                    return {std::nullopt};
                            }
                        }));
            }
        }
    }

  protected:
    DeviceIndex maxCpuNumber;
    std::shared_ptr<PeciCommandsIf> peciCommands;
    std::map<std::pair<DeviceIndex, SensorReadingType>,
             std::future<std::optional<ValueType>>>
        futureSamples;

    void reportStatus(nlohmann::json& out) const override
    {
        for (std::shared_ptr<SensorReadingIf> sensorReading : readings)
        {
            nlohmann::json tmp;
            tmp["Status"] =
                enumToStr(sensorReadingStatusNames, sensorReading->getStatus());
            tmp["Health"] = enumToStr(healthNames, sensorReading->getHealth());
            auto type = enumToStr(kSensorReadingTypeNames,
                                  sensorReading->getSensorReadingType());
            tmp["DeviceIndex"] = sensorReading->getDeviceIndex();
            std::visit([&tmp](auto&& value) { tmp["Value"] = value; },
                       sensorReading->getValue());
            out["Sensors-peci"][type].push_back(tmp);
        }
    }

  private:
    bool isEndpointAvailable(const DeviceIndex index) const
    {
        return sensorReadingsManager->isPowerStateOn() &&
               sensorReadingsManager->isCpuAvailable(index);
    }

    void installSensorReadings()
    {
        for (DeviceIndex cpuIndex = 0; cpuIndex < maxCpuNumber; ++cpuIndex)
        {
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::cpuPackageId, cpuIndex));

            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::prochotRatioCapabilitiesMin, cpuIndex));
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::prochotRatioCapabilitiesMax, cpuIndex));

            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::turboRatioCapabilitiesMin, cpuIndex));
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::turboRatioCapabilitiesMax, cpuIndex));
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::cpuDieMask, cpuIndex));
        }
    }
};

} // namespace nodemanager
