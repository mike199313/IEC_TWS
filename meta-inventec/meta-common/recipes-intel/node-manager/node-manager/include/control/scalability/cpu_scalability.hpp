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

#include "proportional_scalability.hpp"

namespace nodemanager
{

const auto kMaxCpuFactorReassign = 0.25;
const auto kMaxCpuFrequencyDiff = 100;
const auto kCpuLimitAchievedMargin = 0.025;
const auto kCpuLimitChangedMargin = 0.025;

class CpuScalability
    : public ProportionalScalability<kMaxCpuNumber, ReadingType::cpuPresence>
{
  public:
    CpuScalability() = delete;
    CpuScalability(const CpuScalability&) = delete;
    CpuScalability& operator=(const CpuScalability&) = delete;
    CpuScalability(CpuScalability&&) = delete;
    CpuScalability& operator=(CpuScalability&&) = delete;

    CpuScalability(std::shared_ptr<DevicesManagerIf> devicesManagerArg,
                   bool optimizationEnabledArg) :
        ProportionalScalability(devicesManagerArg),
        optimizationEnabled(optimizationEnabledArg)
    {
        if (optimizationEnabled)
        {
            for (DeviceIndex i = 0; i < kMaxCpuNumber; ++i)
            {
                frequencyReadings.push_back(
                    std::make_shared<ReadingEvent>([this, i](double value) {
                        frequenciesPerDevice[i] = value;
                    }));
                devicesManager->registerReadingConsumer(
                    frequencyReadings.back(), ReadingType::cpuAverageFrequency,
                    i);
                capabilitiesReadings.push_back(
                    std::make_shared<ReadingEvent>([this, i](double value) {
                        const auto previous = capabilitiesMax[i];
                        capabilitiesMax[i] = value;
                        if (!std::isnan(capabilitiesMax[i]) &&
                            capabilitiesMax[i] != previous)
                        {
                            reset();
                        }
                    }));
                devicesManager->registerReadingConsumer(
                    capabilitiesReadings.back(),
                    ReadingType::cpuPackagePowerCapabilitiesMax, i);
            }
            limitReading = std::make_shared<ReadingEvent>([this](double value) {
                const auto previousLimit = limit;
                limit = value;
                if (std::abs(limit - previousLimit) >
                    kCpuLimitChangedMargin * previousLimit)
                {
                    reset();
                }
            });
            devicesManager->registerReadingConsumer(
                limitReading, ReadingType::cpuPackagePowerLimit);
            powerReading = std::make_shared<ReadingEvent>(
                [this](double value) { power = value; });
            devicesManager->registerReadingConsumer(
                powerReading, ReadingType::cpuPackagePower);
        }
    }

    ~CpuScalability()
    {
        if (optimizationEnabled)
        {
            for (const auto& reading : frequencyReadings)
            {
                devicesManager->unregisterReadingConsumer(reading);
            }
            for (const auto& reading : capabilitiesReadings)
            {
                devicesManager->unregisterReadingConsumer(reading);
            }
            devicesManager->unregisterReadingConsumer(limitReading);
            devicesManager->unregisterReadingConsumer(powerReading);
        }
    }

    virtual std::vector<double> getFactors() override
    {
        std::vector<double> proportionalFactors =
            ProportionalScalability::getFactors();

        if (!optimizationEnabled || isAnyTurboRatioSet())
        {
            return proportionalFactors;
        }

        if (std::all_of(factors.begin(), factors.end(),
                        [](double v) { return v == 0; }))
        {
            factors = proportionalFactors;
        }
        if ((power < (1.0 - kCpuLimitAchievedMargin) * limit) ||
            (power > (1.0 + kCpuLimitAchievedMargin) * limit))
        {
            return factors;
        }

        const auto& minFreqIt = std::min_element(
            frequenciesPerDevice.begin(), frequenciesPerDevice.end(),
            [](const double a, const double b) {
                return (a == 0) ? false : a < b;
            });

        const auto& maxFreqIt = std::max_element(frequenciesPerDevice.begin(),
                                                 frequenciesPerDevice.end());

        if ((*maxFreqIt - *minFreqIt) > kMaxCpuFrequencyDiff)
        {
            auto minIdx = minFreqIt - frequenciesPerDevice.begin();
            auto maxIdx = maxFreqIt - frequenciesPerDevice.begin();
            if (((factors[maxIdx] - factorChange) >=
                 ((1.0 - kMaxCpuFactorReassign) *
                  proportionalFactors[maxIdx])) &&
                (factors[minIdx] + factorChange <= maxFactors[minIdx]))
            {
                factors[maxIdx] -= factorChange;
                factors[minIdx] += factorChange;
            }
        }
        return factors;
    }

  private:
    bool optimizationEnabled;
    std::array<double, kMaxCpuNumber> frequenciesPerDevice{};
    std::array<double, kMaxCpuNumber> capabilitiesMax{};
    std::vector<double> factors{};
    std::array<double, kMaxCpuNumber> maxFactors{};
    double factorChange{};
    double limit = std::numeric_limits<double>::quiet_NaN();
    double power = std::numeric_limits<double>::quiet_NaN();
    std::shared_ptr<ReadingConsumer> limitReading;
    std::shared_ptr<ReadingConsumer> powerReading;
    std::vector<std::shared_ptr<ReadingConsumer>> capabilitiesReadings;
    std::vector<std::shared_ptr<ReadingConsumer>> frequencyReadings;

    virtual void onPresenceReading(double value) override
    {
        const auto previousPresence = presenceMap;
        ProportionalScalability::onPresenceReading(value);
        if (presenceMap != previousPresence)
        {
            reset();
        }
    }

    bool isAnyTurboRatioSet()
    {
        for (DeviceIndex index = 0; index < kMaxCpuNumber; index++)
        {
            if (!presenceMap[index])
            {
                continue;
            }
            if (devicesManager->isKnobSet(KnobType::TurboRatioLimit, index))
            {
                return true;
            }
        }
        return false;
    }

    void reset()
    {
        factors = ProportionalScalability::getFactors();
        factorChange = 1.0 / limit;
        for (DeviceIndex i = 0; i < kMaxCpuNumber; i++)
        {
            maxFactors[i] = capabilitiesMax[i] / limit;
        }
    }
};

} // namespace nodemanager