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

#include "action.hpp"
#include "common_types.hpp"
#include "statistics/average.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>

namespace nodemanager
{

/**
 * @brief Responsible for providing information about the Trigger Action Type
 *        detected after feeding it with a CPU utilization value.
 *        The provided value is averaged with the kCorrectionTime time
 *        window. The threshold value is specified by the kThreshold
 *        constant and can't be set by a user.
 */
class ActionCpuUtilization : public Action
{
  public:
    ActionCpuUtilization() = delete;
    ActionCpuUtilization(const ActionCpuUtilization&) = delete;
    ActionCpuUtilization& operator=(const ActionCpuUtilization&) = delete;
    ActionCpuUtilization(ActionCpuUtilization&&) = delete;
    ActionCpuUtilization& operator=(ActionCpuUtilization&&) = delete;

    ActionCpuUtilization(double valueArg, std::shared_ptr<Average> averageArg) :
        Action(valueArg)
    {
        average = averageArg;
    }

    virtual ~ActionCpuUtilization() = default;

    /**
     * @brief Adds a new value to a Moving Average object and checks
     *        current average value. Returns a proper Trigger Action Type
     *        based on the average.
     *
     * @param newReading New reading value.
     * @return           Returns Trigger Action Type or nullopt.
     */
    std::optional<TriggerActionType> updateReading(double newReading) override
    {
        std::optional<TriggerActionType> result = std::nullopt;
        average->addSample(newReading);
        auto currentAverage = average->getAvg();
        if (currentAverage > referenceValue && reading <= referenceValue)
        {
            result = TriggerActionType::deactivate;
        }
        else if (currentAverage < referenceValue && reading >= referenceValue)
        {
            result = TriggerActionType::trigger;
        }
        reading = currentAverage;
        return result;
    }

  private:
    std::shared_ptr<Average> average;
};

} // namespace nodemanager
