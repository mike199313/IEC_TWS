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
#include "moving_average.hpp"

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <chrono>
#include <numeric>

namespace nodemanager
{

class PolicyAccumulator : public AccumulatorIf
{
  public:
    PolicyAccumulator(const DurationMs& period)
    {
        userDefinedMovingAverage = std::make_unique<MovingAverage>(period);
        oneSecondMovingAverage = std::make_unique<MovingAverage>(ONE_SECOND);
    }

    virtual ~PolicyAccumulator() = default;

    void addSample(double sample) override
    {
        userDefinedMovingAverage->addSample(sample);
        oneSecondMovingAverage->addSample(sample);
    }

    double getAvg() const override
    {
        return userDefinedMovingAverage->getAvg();
    }

    double getMin() const override
    {
        return userDefinedMovingAverage->getMin();
    }

    double getMax() const override
    {
        return userDefinedMovingAverage->getMax();
    }

    double getCurrentValue() const
    {
        return oneSecondMovingAverage->getAvg();
    }

    DurationMs getStatisticsReportingPeriod() const override
    {
        return userDefinedMovingAverage->getStatisticsReportingPeriod();
    }

    virtual void reset() override
    {
        userDefinedMovingAverage->reset();
        oneSecondMovingAverage->reset();
    }

  private:
    std::unique_ptr<MovingAverage> userDefinedMovingAverage;
    std::unique_ptr<MovingAverage> oneSecondMovingAverage;
};
} // namespace nodemanager