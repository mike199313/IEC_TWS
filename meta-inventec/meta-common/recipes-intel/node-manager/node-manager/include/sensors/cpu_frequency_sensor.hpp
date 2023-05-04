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
#include "common_types.hpp"
#include "peci/peci_commands.hpp"
#include "peci/peci_types.hpp"
#include "peci_sample_sensor.hpp"
#include "sensor_reading.hpp"
#include "sensor_readings_manager.hpp"
#include "utility/performance_monitor.hpp"

#include <future>
#include <optional>
#include <ratio>
#include <sstream>

#include "peci.h"

namespace nodemanager
{

static constexpr auto kFreqMultipiler = 100;
static constexpr auto kC0IdleLevel = 0.8;
static constexpr size_t kNeededRatiosUpdateDivider = 1000U;

class CpuFrequencySensor : public PeciSampleSensor
{
  public:
    CpuFrequencySensor(const CpuFrequencySensor&) = delete;
    CpuFrequencySensor& operator=(const CpuFrequencySensor&) = delete;
    CpuFrequencySensor(CpuFrequencySensor&&) = delete;
    CpuFrequencySensor& operator=(CpuFrequencySensor&&) = delete;

    CpuFrequencySensor(const std::shared_ptr<SensorReadingsManagerIf>&
                           sensorReadingsManagerArg,
                       const std::shared_ptr<PeciCommandsIf>& peciCommandsArg,
                       DeviceIndex maxCpuNumberArg) :
        PeciSampleSensor(sensorReadingsManagerArg, peciCommandsArg,
                         maxCpuNumberArg)
    {
        installSensorReadings();
    }

    virtual ~CpuFrequencySensor() = default;

    void run() override final
    {
        auto perf = Perf("SensorSet-Frequency-run-duration",
                         std::chrono::milliseconds{10});
        for (const auto& sensorReading : readings)
        {
            DeviceIndex deviceIndex = sensorReading->getDeviceIndex();
            if (!isEndpointAvailable(deviceIndex))
            {
                sensorReading->setStatus(SensorReadingStatus::unavailable);
                currentSamples[deviceIndex] = {Clock::now(), std::nullopt};
                previousSamples[deviceIndex] = currentSamples[deviceIndex];
                continue;
            }

            runNeededRatios(deviceIndex);

            auto futureIt = futureSamples.find(deviceIndex);
            if (futureIt != futureSamples.end())
            {
                if (futureIt->second.valid() &&
                    futureIt->second.wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                {
                    currentSamples[deviceIndex] = futureIt->second.get();
                    if (const auto& freq = getCpuFrequency(deviceIndex))
                    {
                        sensorReading->updateValue(static_cast<double>(*freq));
                        sensorReading->setStatus(SensorReadingStatus::valid);
                    }
                    else
                    {
                        sensorReading->setStatus(SensorReadingStatus::invalid);
                    }
                    previousSamples[deviceIndex] = currentSamples[deviceIndex];
                }
                else
                {
                    sensorReading->setStatus(SensorReadingStatus::unavailable);
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
                                    peciIf->getC0CounterSensor(deviceIndex)};
                        }));
            }
        }
    }

  protected:
    using NeededRatios =
        std::tuple<std::optional<uint64_t>, std::optional<uint64_t>,
                   std::optional<uint64_t>>;
    std::unordered_map<DeviceIndex, std::future<PeciSample>> futureSamples;
    std::unordered_map<DeviceIndex, std::future<NeededRatios>>
        futuresNeededRatios;

  private:
    std::optional<uint64_t> coreCount = std::nullopt;
    double pnFrequency;
    double freqIdleLevel;
    size_t counter = 0;

    void runNeededRatios(const DeviceIndex deviceIndex)
    {
        static std::optional<uint64_t> maxEfficiencyRatio = std::nullopt;
        static std::optional<uint64_t> minOperatingRatio = std::nullopt;
        std::optional<uint32_t> cpuId = std::nullopt;
        if (auto sensorReadingCpuId =
                sensorReadingsManager->getAvailableAndValueValidSensorReading(
                    SensorReadingType::cpuPackageId, deviceIndex))
        {
            if (std::holds_alternative<uint32_t>(
                    sensorReadingCpuId->getValue()))
            {
                cpuId = std::get<uint32_t>(sensorReadingCpuId->getValue());
            }
        }

        auto& futureNeededRatios = futuresNeededRatios[deviceIndex];
        if (futureNeededRatios.valid() &&
            futureNeededRatios.wait_for(std::chrono::seconds(0)) ==
                std::future_status::ready)
        {
            const auto [maxEfficiencyRatioNew, minOperatingRatioNew,
                        coreCountNew] = futureNeededRatios.get();
            if (maxEfficiencyRatioNew)
            {
                maxEfficiencyRatio = maxEfficiencyRatioNew;
                pnFrequency =
                    static_cast<double>(*maxEfficiencyRatio) * kFreqMultipiler;
            }
            if (minOperatingRatioNew)
            {
                minOperatingRatio = minOperatingRatioNew;
                freqIdleLevel = static_cast<double>(*minOperatingRatio) *
                                kFreqMultipiler * kC0IdleLevel;
            }
            if (coreCountNew)
            {
                coreCount = coreCountNew;
            }
        }

        // Needed ratios do not change frequently and can be
        // fetched once per every kNeededRatiosUpdateDivider
        // call or when previous call has failed.
        if ((counter++ % kNeededRatiosUpdateDivider == 0) ||
            (!maxEfficiencyRatio || !minOperatingRatio || !coreCount))
        {
            if (!futureNeededRatios.valid())
            {
                futureNeededRatios = std::move(std::async(
                    std::launch::async,
                    [deviceIndex, cpuId,
                     peciIf = peciCommands]() -> NeededRatios {
                        if (cpuId)
                        {
                            return CpuFrequencySensor::getNeededRatios(
                                deviceIndex, *cpuId, peciIf);
                        }
                        return NeededRatios{};
                    }));
            }
        }
    }

    static NeededRatios
        getNeededRatios(const DeviceIndex cpuIndex, const uint32_t cpuId,
                        const std::shared_ptr<PeciCommandsIf>& peciIf)
    {

        const auto maxEfficencyRatio =
            peciIf->getMaxEfficiencyRatio(cpuIndex, cpuId);
        const auto minOperatingRatio =
            peciIf->getMinOperatingRatio(cpuIndex, cpuId);
        const auto detectedCoreCount = peciIf->detectCores(cpuIndex, cpuId);

        if (maxEfficencyRatio && minOperatingRatio && detectedCoreCount)
        {
            return std::make_tuple(*maxEfficencyRatio, *minOperatingRatio,
                                   *detectedCoreCount);
        }
        return std::make_tuple(std::nullopt, std::nullopt, std::nullopt);
    }

    std::optional<double> getCpuFrequency(const DeviceIndex cpuIndex)
    {
        const auto deltas = getSampleDeltas(cpuIndex);
        if (deltas && coreCount)
        {
            const auto [freqDelta, duration] = *deltas;
            const auto timeDelta = duration.count();
            if (timeDelta == 0)
            {
                Logger::log<LogLevel::debug>("Cannot calculate cpu frequency, "
                                             "sample_duration is zero");
                return std::nullopt;
            }
            if (*coreCount == 0)
            {
                Logger::log<LogLevel::debug>("Cannot calculate cpu frequency, "
                                             "number of cores is zero");
                return std::nullopt;
            }
            double ret = static_cast<double>(freqDelta) /
                         (static_cast<double>(*coreCount) *
                          static_cast<double>(timeDelta));

            if (ret < freqIdleLevel)
            {
                ret = pnFrequency;
            }
            Logger::log<LogLevel::debug>(
                "freqDelta: %ld, coreCount: %d, "
                "sample_duration: %ld[us], frequency: %d",
                freqDelta, static_cast<unsigned>(*coreCount), timeDelta, ret);
            return ret;
        }
        return std::nullopt;
    }

    void installSensorReadings()
    {
        for (DeviceIndex cpuIndex = 0; cpuIndex < maxCpuNumber; ++cpuIndex)
        {
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::cpuAverageFrequency, cpuIndex));
        }
    }
}; // namespace nodemanager

} // namespace nodemanager
