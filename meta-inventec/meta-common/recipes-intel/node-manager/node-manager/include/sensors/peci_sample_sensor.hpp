/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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

namespace nodemanager
{
using PeciSample = std::tuple<Clock::time_point, std::optional<uint64_t>>;

class PeciSampleSensor : public Sensor
{
  public:
    PeciSampleSensor() = delete;
    PeciSampleSensor(const PeciSampleSensor&) = delete;
    PeciSampleSensor& operator=(const PeciSampleSensor&) = delete;
    PeciSampleSensor(PeciSampleSensor&&) = delete;
    PeciSampleSensor& operator=(PeciSampleSensor&&) = delete;

    PeciSampleSensor(
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
        std::shared_ptr<PeciCommandsIf> peciCommandsArg,
        DeviceIndex maxCpuNumberArg) :
        Sensor(sensorReadingsManagerArg),
        peciCommands(peciCommandsArg), maxCpuNumber(maxCpuNumberArg)
    {
    }

    virtual ~PeciSampleSensor() = default;

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

  protected:
    std::shared_ptr<PeciCommandsIf> peciCommands;
    DeviceIndex maxCpuNumber;
    std::unordered_map<DeviceIndex, PeciSample> previousSamples;
    std::unordered_map<DeviceIndex, PeciSample> currentSamples;

    /**
     * @brief Get delta between current and previous sample
     * values and timestamps for the specified cpu index.
     */
    std::optional<std::tuple<uint64_t, std::chrono::microseconds>>
        getSampleDeltas(const DeviceIndex cpuIndex)
    {
        if (cpuIndex >= previousSamples.size() ||
            cpuIndex >= currentSamples.size())
        {
            Logger::log<LogLevel::error>(
                "PeciSampleSensor, invalid value of cpuIndex");
            return std::nullopt;
        }

        const auto [previousTimestamp, previousValue] =
            previousSamples[cpuIndex];
        const auto [currentTimestamp, currentValue] = currentSamples[cpuIndex];

        if (currentValue.has_value() && previousValue.has_value())
        {
            return std::make_tuple(
                *currentValue - *previousValue,
                std::chrono::duration_cast<std::chrono::microseconds>(
                    currentTimestamp - previousTimestamp));
        }

        return std::nullopt;
    }

    bool isEndpointAvailable(const DeviceIndex index) const
    {
        return sensorReadingsManager->isPowerStateOn() &&
               sensorReadingsManager->isCpuAvailable(index);
    }
};

} // namespace nodemanager