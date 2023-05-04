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

#include "triggers/trigger_enums.hpp"

#include <optional>

namespace nodemanager
{

class ActionIf
{
  public:
    virtual ~ActionIf() = default;
    virtual std::optional<TriggerActionType>
        updateReading(double newReading) = 0;
};

/**
 * @brief Responsible for providing information about the Trigger Action Type
 *        detected after feeding it with a reading value.
 */
class Action : public ActionIf
{
  public:
    Action() = delete;
    Action(const Action&) = delete;
    Action& operator=(const Action&) = delete;
    Action(Action&&) = delete;
    Action& operator=(Action&&) = delete;

    Action(double valueArg) : referenceValue(valueArg), reading(valueArg)
    {
    }

    /**
     * @brief Updates value and checks what Trigger Action Type a new value
     *        triggers. The detected Trigger Action Type is returned.
     *
     * @param newReading  New reading value.
     * @return            Returns Trigger Action Type or nullopt.
     */
    virtual std::optional<TriggerActionType> updateReading(double newReading)
    {
        std::optional<TriggerActionType> result = std::nullopt;
        if (std::isnan(newReading))
        {
            return std::nullopt;
        }
        if (newReading > referenceValue && reading <= referenceValue)
        {
            result = TriggerActionType::trigger;
        }
        else if (newReading < referenceValue && reading >= referenceValue)
        {
            result = TriggerActionType::deactivate;
        }
        reading = newReading;
        return result;
    }

  protected:
    double referenceValue;
    double reading;
};

} // namespace nodemanager