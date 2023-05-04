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

#include <iostream>
#include <map>
#include <variant>

namespace nodemanager
{
using StatValuesMap =
    std::map<std::string, std::variant<double, uint32_t, uint64_t, bool>>;

class StatisticIf : public ReadingConsumer
{
  public:
    virtual ~StatisticIf() = default;
    virtual void reset() = 0;
    virtual StatValuesMap getValuesMap() const = 0;
    virtual const std::string& getName() const = 0;
    virtual void enableStatisticCalculation() = 0;
    virtual void disableStatisticCalculation() = 0;
    void reportEvent(SensorEventType eventType, const SensorContext& sensorCtx,
                     const ReadingContext& readingCtx)
    {
    }
    void reportEvent(const ReadingEventType eventType,
                     const ReadingContext& readingCtx)
    {
    }
};

} // namespace nodemanager