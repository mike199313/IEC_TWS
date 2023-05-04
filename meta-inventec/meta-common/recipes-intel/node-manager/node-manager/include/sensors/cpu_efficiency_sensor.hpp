/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2021 Intel Corporation.
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
#include "peci/peci_commands.hpp"
#include "peci/peci_types.hpp"
#include "peci_sample_sensor.hpp"
#include "sensor_reading.hpp"
#include "sensor_readings_manager.hpp"
#include "utility/performance_monitor.hpp"

#include <future>
#include <iostream>
#include <optional>

#include "peci.h"

namespace nodemanager
{

/**
 * @brief Creates Sensor Readings describing CPU efficiency, based on EPI
 * counter readings. Uses PECI interface.
 */
class CpuEfficiencySensor : public PeciSampleSensor
{
  public:
    CpuEfficiencySensor(const CpuEfficiencySensor&) = delete;
    CpuEfficiencySensor& operator=(const CpuEfficiencySensor&) = delete;
    CpuEfficiencySensor(CpuEfficiencySensor&&) = delete;
    CpuEfficiencySensor& operator=(CpuEfficiencySensor&&) = delete;

    CpuEfficiencySensor(
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
        std::shared_ptr<PeciCommandsIf> peciCommandsArg,
        DeviceIndex maxCpuNumberArg) :
        PeciSampleSensor(sensorReadingsManagerArg, peciCommandsArg,
                         maxCpuNumberArg)
    {
        installSensorReadings();
    }

    virtual ~CpuEfficiencySensor() = default;

    void run() override final
    {
        auto perf = Perf("SensorSet-Efficiency-run-duration",
                         std::chrono::milliseconds{10});
        for (const auto& epiSensorReading : readings)
        {
            DeviceIndex deviceIndex = epiSensorReading->getDeviceIndex();
            if (!isEndpointAvailable(deviceIndex))
            {
                currentSamples[deviceIndex] = {Clock::now(), std::nullopt};
                epiSensorReading->setStatus(SensorReadingStatus::unavailable);
                previousSamples[deviceIndex] = currentSamples[deviceIndex];
                continue;
            }

            auto futureIt = futureSamples.find(deviceIndex);
            if (futureIt != futureSamples.end())
            {
                if (futureIt->second.valid() &&
                    futureIt->second.wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                {
                    currentSamples[deviceIndex] = futureIt->second.get();
                    if (const auto& epi = getCpuEfficiency(deviceIndex))
                    {
                        epiSensorReading->updateValue(
                            static_cast<double>(*epi));
                        epiSensorReading->setStatus(SensorReadingStatus::valid);
                    }
                    else
                    {
                        epiSensorReading->setStatus(
                            SensorReadingStatus::invalid);
                    }
                    previousSamples[deviceIndex] = currentSamples[deviceIndex];
                }
                else
                {
                    epiSensorReading->setStatus(
                        SensorReadingStatus::unavailable);
                }
            }

            if (futureIt == futureSamples.end() ||
                futureIt->second.valid() == false)
            {
                futureSamples.insert_or_assign(
                    deviceIndex,
                    std::async(
                        std::launch::async,
                        [deviceIndex, peciIf = peciCommands]() -> PeciSample {
                            return {Clock::now(),
                                    peciIf->getEpiCounterSensor(deviceIndex)};
                        }));
            }
        }
    }

  protected:
    std::map<DeviceIndex, std::future<PeciSample>> futureSamples;

  private:
    /**
     * @brief Calculates CPU efficiency by simply taking EPI counter delta
     * value and dividing by time delta.
     *
     * @param cpuIndex
     * @return If some sample stored in the input vectors is invalid, returns
     * nullopt. Otherwise calculated CPU efficiency for the specified CPU
     * index.
     */
    std::optional<double> getCpuEfficiency(const DeviceIndex cpuIndex)
    {
        const auto deltas = getSampleDeltas(cpuIndex);
        if (deltas)
        {
            const auto [epiDelta, duration] = *deltas;
            const auto timeDelta = duration.count();
            if (timeDelta == 0)
            {
                Logger::log<LogLevel::debug>("Cannot calculate cpu efficiency, "
                                             "sample_duration is zero!");
                return std::nullopt;
            }
            double ret =
                static_cast<double>(epiDelta) / static_cast<double>(timeDelta);
            Logger::log<LogLevel::debug>(
                "epiDelta: %ld, "
                "sample_duration: %ld[us], efficiency: %d",
                epiDelta, timeDelta, ret);
            return ret;
        }
        return std::nullopt;
    }

    void installSensorReadings()
    {
        for (DeviceIndex cpuIndex = 0; cpuIndex < maxCpuNumber; ++cpuIndex)
        {
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::cpuEfficiency, cpuIndex));
        }
    }
};

} // namespace nodemanager