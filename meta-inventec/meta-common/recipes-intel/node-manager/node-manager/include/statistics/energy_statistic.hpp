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

#include "clock.hpp"
#include "global_accumulator.hpp"
#include "policy_accumulator.hpp"
#include "readings/reading_consumer.hpp"
#include "statistic_if.hpp"
#include "utility/ranges.hpp"

namespace nodemanager
{

class EnergyStatistic : public StatisticIf
{
  public:
    EnergyStatistic(const EnergyStatistic&) = delete;
    EnergyStatistic& operator=(const EnergyStatistic&) = delete;
    EnergyStatistic(EnergyStatistic&&) = delete;
    EnergyStatistic& operator=(EnergyStatistic&&) = delete;

    EnergyStatistic(std::string nameArg) : name(std::move(nameArg))
    {
    }

    virtual ~EnergyStatistic() = default;

    virtual void updateValue(double newValue) override
    {
        Clock::time_point timestamp = Clock::now();
        if (!std::isnan(newValue))
        {
            double integralPart;

            islastSampleOk = true;
            leftover += modf(newValue, &integralPart);
            accumulatedValue += static_cast<uint64_t>(integralPart);

            if (leftover > 1)
            {
                leftover = modf(leftover, &integralPart);
                accumulatedValue += static_cast<uint64_t>(integralPart);
            }
        }
        else
        {
            islastSampleOk = false;
        }

        totalElapsedTime += timestamp - lastTimestamp;
        lastTimestamp = timestamp;
    }

    virtual void reset() override
    {
        totalElapsedTime = std::chrono::duration<double, std::milli>(0);
        lastTimestamp = Clock::now();
        accumulatedValue = 0;
        leftover = 0.0;
    }

    virtual StatValuesMap getValuesMap() const override
    {
        StatValuesMap stats{};
        uint32_t statReportingPeriod =
            std::numeric_limits<uint32_t>::quiet_NaN();
        const auto duration =
            std::chrono::duration_cast<std::chrono::seconds>(totalElapsedTime)
                .count();

        if (isCastSafe<uint32_t>(duration))
        {
            statReportingPeriod = static_cast<uint32_t>(duration);
        }

        stats = {{"Current", accumulatedValue},
                 {"StatisticsReportingPeriod", statReportingPeriod},
                 {"MeasurementState", islastSampleOk}};

        return stats;
    }

    virtual const std::string& getName() const override
    {
        return name;
    }

    virtual void enableStatisticCalculation() override
    {
        throw std::logic_error(
            "energy statistics dont support enable statistics");
    }

    virtual void disableStatisticCalculation() override
    {
        throw std::logic_error(
            "energy statistics dont support disable statistics");
    }

  private:
    std::chrono::duration<double, std::milli> totalElapsedTime =
        std::chrono::duration<double, std::milli>(0);
    Clock::time_point lastTimestamp = Clock::now();
    uint64_t accumulatedValue = 0;
    double leftover = 0.0;
    bool islastSampleOk = false;
    std::string name;
};

} // namespace nodemanager
