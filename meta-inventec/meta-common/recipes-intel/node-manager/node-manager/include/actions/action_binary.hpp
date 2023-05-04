/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2022 Intel Corporation.
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
#include "utility/ranges.hpp"

namespace nodemanager
{

class ActionBinary : public Action
{
  public:
    ActionBinary() = delete;
    ActionBinary(const ActionBinary&) = delete;
    ActionBinary& operator=(const ActionBinary&) = delete;
    ActionBinary(ActionBinary&&) = delete;
    ActionBinary& operator=(ActionBinary&&) = delete;

    ActionBinary(double valueArg) : Action(valueArg)
    {
    }

    virtual ~ActionBinary() = default;

    std::optional<TriggerActionType> updateReading(double newReading) override
    {
        std::optional<TriggerActionType> result = std::nullopt;

        if (isCastSafe<bool>(newReading))
        {
            if (newReading > reading)
            {
                result = TriggerActionType::trigger;
            }
            else if (newReading < reading)
            {
                result = TriggerActionType::deactivate;
            }
            reading = newReading;
        }

        return result;
    }
};

} // namespace nodemanager