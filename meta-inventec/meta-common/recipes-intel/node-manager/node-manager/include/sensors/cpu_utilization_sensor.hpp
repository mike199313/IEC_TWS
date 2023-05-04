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

static constexpr uint8_t kMaxUtilizationDivider = 100U;
static constexpr auto kHundredPercent = 100;

/**
 * @brief Sensor measuring CPU utilization based on C0 residency.
 * Uses peci interface.
 */
class CpuUtilizationSensor : public PeciSampleSensor
{
  public:
    CpuUtilizationSensor(const CpuUtilizationSensor&) = delete;
    CpuUtilizationSensor& operator=(const CpuUtilizationSensor&) = delete;
    CpuUtilizationSensor(CpuUtilizationSensor&&) = delete;
    CpuUtilizationSensor& operator=(CpuUtilizationSensor&&) = delete;

    CpuUtilizationSensor(const std::shared_ptr<SensorReadingsManagerIf>&
                             sensorReadingsManagerArg,
                         const std::shared_ptr<PeciCommandsIf>& peciCommandsArg,
                         DeviceIndex maxCpuNumberArg) :
        PeciSampleSensor(sensorReadingsManagerArg, peciCommandsArg,
                         maxCpuNumberArg)
    {
        installSensorReadings();
    }

    virtual ~CpuUtilizationSensor() = default;

    void runMaxCpuUtilization(const DeviceIndex deviceIndex)
    {
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

        auto& futureMaxCpu = futuresMaxCpu[deviceIndex];
        if (futureMaxCpu.valid() && futureMaxCpu.wait_for(std::chrono::seconds(
                                        0)) == std::future_status::ready)
        {
            const auto newMax = futureMaxCpu.get();
            if (newMax)
            {
                maxCpuUtilization = newMax;
            }
        }

        // Max values do not change frequently and can be
        // fetched once per every kMaxUtilizationDivider
        // call or when previous call has failed.
        if ((++counter % kMaxUtilizationDivider == 0) || !maxCpuUtilization)
        {
            counter = 0;
            if (!futureMaxCpu.valid())
            {
                futureMaxCpu = std::move(std::async(
                    std::launch::async,
                    [deviceIndex, cpuId,
                     peciIf = peciCommands]() -> std::optional<uint64_t> {
                        if (cpuId)
                        {
                            return CpuUtilizationSensor::getMaxCpuUtilization(
                                deviceIndex, *cpuId, peciIf);
                        }
                        else
                        {
                            return std::nullopt;
                        }
                    }));
            }
        }
    }

    void run() override final
    {
        auto perf = Perf("SensorSet-Utilization-run-duration",
                         std::chrono::milliseconds{10});
        for (const auto& sensor : readings)
        {
            DeviceIndex deviceIndex = sensor->getDeviceIndex();
            if (!isEndpointAvailable(deviceIndex))
            {
                sensor->setStatus(SensorReadingStatus::unavailable);
                currentSamples[deviceIndex] = {Clock::now(), std::nullopt};
                previousSamples[deviceIndex] = currentSamples[deviceIndex];
                continue;
            }

            runMaxCpuUtilization(deviceIndex);

            auto futureIt = futureSamples.find(deviceIndex);
            if (futureIt != futureSamples.end())
            {
                if (futureIt->second.valid() &&
                    futureIt->second.wait_for(std::chrono::seconds(0)) ==
                        std::future_status::ready)
                {
                    currentSamples[deviceIndex] = futureIt->second.get();
                    const auto deltas = getSampleDeltas(deviceIndex);
                    if (deltas && maxCpuUtilization)
                    {
                        const auto [c0Delta, duration] = *deltas;
                        sensor->updateValue(CpuUtilizationType{
                            c0Delta, duration, *maxCpuUtilization});
                        sensor->setStatus(SensorReadingStatus::valid);
                    }
                    else
                    {
                        sensor->setStatus(SensorReadingStatus::invalid);
                    }
                    previousSamples[deviceIndex] = currentSamples[deviceIndex];
                }
                else
                {
                    sensor->setStatus(SensorReadingStatus::unavailable);
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
    std::unordered_map<DeviceIndex, std::future<PeciSample>> futureSamples;
    std::unordered_map<DeviceIndex, std::future<std::optional<uint64_t>>>
        futuresMaxCpu;

  private:
    std::optional<uint64_t> maxCpuUtilization = std::nullopt;
    uint8_t counter = 0;

    /**
     * @brief Get the Max CPU Utilization value.
     *
     * @param cpuIndex
     * @param peciIf - peci interface
     * @return std::optional<uint64_t> Max CPU Utilization value [MHz]
     */
    static std::optional<uint64_t>
        getMaxCpuUtilization(const DeviceIndex cpuIndex, const uint32_t cpuId,
                             const std::shared_ptr<PeciCommandsIf>& peciIf)
    {
        const auto isTurbo = peciIf->isTurboEnabled(cpuIndex, cpuId);

        if (isTurbo)
        {
            if (const auto coreCount = peciIf->detectCores(cpuIndex, cpuId))
            {
                if (const auto frequencyMHz = getMaxFrequncy(
                        cpuIndex, cpuId, *isTurbo, *coreCount, peciIf))
                {
                    return frequencyMHz->count() * *coreCount;
                }
            }
        }

        return std::nullopt;
    }

    /**
     * @brief Get the Max CPU Frequncy which depends on the turbo.
     *
     * @param cpuIndex
     * @param isTurbo
     * @param coreCount
     * @param peciIf - peci interface
     * @return std::optional<MHz> max cpu frequency [MHz]
     */
    static std::optional<MHz>
        getMaxFrequncy(const DeviceIndex cpuIndex, const uint32_t cpuId,
                       const bool isTurbo, const uint8_t coreCount,
                       const std::shared_ptr<PeciCommandsIf>& peciIf)
    {
        if (isTurbo)
        {
            return detectMaxTurboFreq(cpuIndex, cpuId, coreCount, peciIf);
        }
        else
        {
            return detectMaxNonTurboFreq(cpuIndex, cpuId, peciIf);
        }
    }

    /**
     * @brief Returns the `turbo` max cpu frequency
     *
     * @param cpuIndex
     * @param coreCount
     * @param peciIf - peci interface
     * @return std::optional<MHz> max cpu frequency [MHz]
     */
    static std::optional<MHz>
        detectMaxTurboFreq(const DeviceIndex cpuIndex, const uint32_t cpuId,
                           const uint8_t coreCount,
                           const std::shared_ptr<PeciCommandsIf>& peciIf)
    {
        if (const auto maxTurboRatio =
                peciIf->detectMinTurboRatio(cpuIndex, cpuId, coreCount))
        {
            return std::chrono::duration_cast<MHz>(HundredMHz{*maxTurboRatio});
        }
        return std::nullopt;
    }

    /**
     * @brief Returns the `non-turbo` max cpu frequency
     *
     * @param cpuIndex
     * @param peciIf - peci interface
     * @return std::optional<MHz> max cpu frequency [MHz]
     */
    static std::optional<MHz>
        detectMaxNonTurboFreq(const DeviceIndex cpuIndex, const uint32_t cpuId,
                              const std::shared_ptr<PeciCommandsIf>& peciIf)
    {
        if (const auto maxNonTurboRatio =
                peciIf->getMaxNonTurboRatio(cpuIndex, cpuId))
        {
            return std::chrono::duration_cast<MHz>(
                HundredMHz{*maxNonTurboRatio});
        }
        return std::nullopt;
    }

    void installSensorReadings()
    {
        for (DeviceIndex cpuIndex = 0; cpuIndex < maxCpuNumber; ++cpuIndex)
        {
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::cpuUtilization, cpuIndex));
        }
    }

    template <class T>
    T counterDiff(const T currentValue, const T prevValue)
    {
        if (currentValue > prevValue)
        {
            return currentValue - prevValue;
        }
        else
        {
            return currentValue + (std::numeric_limits<T>::max() - prevValue);
        }
    }
};

} // namespace nodemanager