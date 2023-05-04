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

class AccumulatorIf
{
  public:
    virtual ~AccumulatorIf() = default;
    virtual void addSample(double sample) = 0;
    virtual double getAvg() const = 0;
    virtual double getMin() const = 0;
    virtual double getMax() const = 0;
    virtual DurationMs getStatisticsReportingPeriod() const = 0;
    virtual void reset() = 0;
    virtual double getCurrentValue() const = 0;
};

} // namespace nodemanager