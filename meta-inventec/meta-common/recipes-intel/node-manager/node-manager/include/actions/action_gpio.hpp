/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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

namespace nodemanager
{

/**
 * @brief Responsible for providing information about the Trigger Action Type
 *        detected after feeding it with a gpio state value.
 */
class ActionGpio : public ActionIf
{
  public:
    ActionGpio(const ActionGpio&) = delete;
    ActionGpio& operator=(const ActionGpio&) = delete;
    ActionGpio(ActionGpio&&) = delete;
    ActionGpio& operator=(ActionGpio&&) = delete;

    ActionGpio(bool triggerOnRisingEdgeArg = true) :
        triggerOnRisingEdge(triggerOnRisingEdgeArg)
    {
        if (triggerOnRisingEdge)
        {
            reading = 0;
        }
        else
        {
            reading = 1;
        }
    }

    virtual ~ActionGpio() = default;

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
        if (newReading == 1 && reading == 0)
        {
            if (triggerOnRisingEdge)
            {
                result = TriggerActionType::trigger;
            }
            else
            {
                result = TriggerActionType::deactivate;
            }
        }
        else if (newReading == 0 && reading == 1)
        {
            if (triggerOnRisingEdge)
            {
                result = TriggerActionType::deactivate;
            }
            else
            {
                result = TriggerActionType::trigger;
            }
        }
        reading = newReading;
        return result;
    }

  protected:
    bool triggerOnRisingEdge;
    double reading;
};

} // namespace nodemanager
