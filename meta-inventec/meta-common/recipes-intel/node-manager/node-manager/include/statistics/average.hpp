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

#include "clock.hpp"

namespace nodemanager
{
using DurationMs = std::chrono::duration<double, std::milli>;
static constexpr DurationMs ONE_SECOND = DurationMs{std::chrono::seconds{1}};
class Average
{
  public:
    virtual ~Average() = default;
    virtual void addSample(double sample) = 0;
    virtual double getAvg() = 0;
    virtual DurationMs getStatisticsReportingPeriod() = 0;
    virtual double getMin() = 0;
    virtual double getMax() = 0;

    virtual void reset()
    {
        isReset = true;
        accTime = DurationMs::zero();
        accReading = 0;
        accMin = std::numeric_limits<double>::max();
        accMax = std::numeric_limits<double>::lowest();
        timestamp = Clock::now();
        lastSample = std::numeric_limits<double>::quiet_NaN();
    }

  protected:
    double accMax{std::numeric_limits<double>::lowest()};
    double accMin{std::numeric_limits<double>::max()};
    DurationMs accTime = DurationMs::zero();
    double accReading{0};
    Clock::time_point timestamp = Clock::now();
    bool isReset{true};
    double lastSample = std::numeric_limits<double>::quiet_NaN();
};
} // namespace nodemanager
