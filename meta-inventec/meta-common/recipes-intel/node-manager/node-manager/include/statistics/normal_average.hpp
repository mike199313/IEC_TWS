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

#include "average.hpp"
#include "clock.hpp"

#include <cmath>

namespace nodemanager
{

class NormalAverage : public Average
{
  public:
    NormalAverage()
    {
    }
    virtual ~NormalAverage() = default;

    void addSample(double sample) override
    {
        isReset = false;
        if (!std::isfinite(sample))
        {
            reset();
            return;
        }

        auto now = Clock::now();
        DurationMs delta = now - timestamp;
        timestamp = now;

        if (std::isfinite(lastSample))
        {
            accReading += lastSample * delta.count();
            accTime += delta;
        }

        accMin = std::min(accMin, sample);
        accMax = std::max(accMax, sample);

        lastSample = sample;
    }

    double getAvg() override
    {
        if (isReset)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        addSample(lastSample);
        return accReading / accTime.count();
    };

    DurationMs getStatisticsReportingPeriod() override
    {
        addSample(lastSample);
        return accTime;
    }

    virtual double getMin() override
    {
        if (isReset)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return accMin;
    }

    virtual double getMax() override
    {
        if (isReset)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return accMax;
    }

    void reset() override
    {
        Average::reset();
    }
};
} // namespace nodemanager
