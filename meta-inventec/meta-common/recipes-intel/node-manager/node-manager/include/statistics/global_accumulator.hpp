/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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

#include "accumulator_if.hpp"
#include "average.hpp"
#include "moving_average.hpp"
#include "normal_average.hpp"

#include <chrono>

namespace nodemanager
{

class GlobalAccumulator : public AccumulatorIf
{
  public:
    GlobalAccumulator()
    {
        lastResetAverage = std::make_unique<NormalAverage>();
        oneSecondMovingAverage = std::make_unique<MovingAverage>(ONE_SECOND);
    }
    virtual ~GlobalAccumulator() = default;

    void addSample(double sample) override
    {
        lastResetAverage->addSample(sample);
        oneSecondMovingAverage->addSample(sample);
    }

    double getAvg() const override
    {
        return lastResetAverage->getAvg();
    }

    double getMin() const override
    {
        return lastResetAverage->getMin();
    }

    double getMax() const override
    {
        return lastResetAverage->getMax();
    }

    double getCurrentValue() const
    {
        return oneSecondMovingAverage->getAvg();
    }

    virtual void reset() override
    {
        lastResetAverage->reset();
        oneSecondMovingAverage->reset();
    }

    DurationMs getStatisticsReportingPeriod() const override
    {
        return lastResetAverage->getStatisticsReportingPeriod();
    }

  private:
    std::unique_ptr<NormalAverage> lastResetAverage;
    std::unique_ptr<MovingAverage> oneSecondMovingAverage;
};
} // namespace nodemanager
