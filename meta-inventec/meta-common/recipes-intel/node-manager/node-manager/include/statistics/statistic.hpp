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

#include "clock.hpp"
#include "global_accumulator.hpp"
#include "policy_accumulator.hpp"
#include "readings/reading_consumer.hpp"
#include "statistic_if.hpp"
#include "utility/ranges.hpp"

namespace nodemanager
{

class Statistic : public StatisticIf
{
  public:
    Statistic(const Statistic&) = delete;
    Statistic& operator=(const Statistic&) = delete;
    Statistic(Statistic&&) = delete;
    Statistic& operator=(Statistic&&) = delete;

    Statistic(std::string nameArg,
              std::shared_ptr<AccumulatorIf> accumulatorArg) :
        accumulator(std::move(accumulatorArg)),
        name(std::move(nameArg))
    {
    }

    virtual ~Statistic() = default;

    virtual void updateValue(double newValue) override
    {
        if (!std::isfinite(newValue))
        {
            isLastSampleOk = false;
            return;
        }

        if (enabled)
        {
            accumulator->addSample(newValue);
            hasFiniteValue = true;
            isLastSampleOk = true;
        }
    }

    virtual void reset() override
    {
        accumulator->reset();
        hasFiniteValue = false;
    }

    virtual StatValuesMap getValuesMap() const override
    {
        StatValuesMap stats{};
        uint32_t statReportingPeriod =
            std::numeric_limits<uint32_t>::quiet_NaN();
        const auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                                  accumulator->getStatisticsReportingPeriod())
                                  .count();
        if (isCastSafe<uint32_t>(duration))
        {
            statReportingPeriod = static_cast<uint32_t>(duration);
        }

        if (!hasFiniteValue)
        {
            stats = {{"Current", std::numeric_limits<double>::quiet_NaN()},
                     {"Max", std::numeric_limits<double>::quiet_NaN()},
                     {"Min", std::numeric_limits<double>::quiet_NaN()},
                     {"Average", std::numeric_limits<double>::quiet_NaN()},
                     {"StatisticsReportingPeriod", statReportingPeriod},
                     {"MeasurementState", false}};
        }
        else
        {
            bool measurementState = enabled && isLastSampleOk;
            stats = {{"Current", accumulator->getCurrentValue()},
                     {"Max", accumulator->getMax()},
                     {"Min", accumulator->getMin()},
                     {"Average", accumulator->getAvg()},
                     {"StatisticsReportingPeriod", statReportingPeriod},
                     {"MeasurementState", measurementState}};
        }

        return stats;
    }

    virtual const std::string& getName() const override
    {
        return name;
    }

    virtual void enableStatisticCalculation() override
    {
        enabled = true;
        reset();
    }

    virtual void disableStatisticCalculation() override
    {
        enabled = false;
    }

  private:
    bool hasFiniteValue = false;
    bool isLastSampleOk = false;
    std::shared_ptr<AccumulatorIf> accumulator;
    Clock::time_point timestamp = Clock::now();
    std::string name;
    bool enabled{true};
};

} // namespace nodemanager
