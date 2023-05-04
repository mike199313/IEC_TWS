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

#include "actions/action.hpp"
#include "readings/reading_consumer.hpp"
#include "readings/reading_type.hpp"
#include "trigger_enums.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/final_callback.hpp"
#include "utility/performance_monitor.hpp"
#include "utility/property.hpp"

namespace nodemanager
{

using TriggerCallback = std::function<void(TriggerActionType)>;

/**
 * @brief Responsible for calling user's callback when the related
 * parameter crosses the provided level or whether reading missed.
 * These two events are reported to the Triggers object owner by
 * a callback provided while Triggers creation. Callback takes single
 * argument which is detected event type. Trigger is the generic class
 * which is parametrized with TriggerType.
 */
class Trigger : public ReadingConsumer
{
  public:
    Trigger() = delete;
    Trigger(const Trigger&) = delete;
    Trigger& operator=(const Trigger&) = delete;
    Trigger(Trigger&&) = delete;
    Trigger& operator=(Trigger&&) = delete;

    Trigger(std::shared_ptr<ActionIf> actionArg, TriggerCallback callbackArg) :
        action(actionArg), callback(std::move(callbackArg))
    {
    }

    virtual ~Trigger() = default;

    /**
     * @brief Updates value in a related Action object and issues
     *        a callback (if provided) based on Action object response
     *        to a new value.
     *
     * @param newValue New reading value.
     */
    void updateValue(double newValue) override
    {
        const auto actionType = action->updateReading(newValue);
        if (callback != nullptr && actionType)
        {
            callback(*actionType);
        }
    }

    /**
     * @brief Issues a callback for a specific event type.
     *
     * @param eventType Sensor event type.
     */
    void reportEvent(SensorEventType eventType, const SensorContext& sensorCtx,
                     const ReadingContext& readingCtx) override
    {
        if (eventType == SensorEventType::readingMissing && callback != nullptr)
        {
            callback(TriggerActionType::missingReading);
        }
    }
    virtual void reportEvent(const ReadingEventType eventType,
                             const ReadingContext& readingCtx) override
    {
    }

    /**
     * @brief Returns a Reading Type realted to a Trigger Type.
     *
     * @param tt Trigger's type.
     * @return   Returns a Reading Type.
     */
    static ReadingType toReadingType(const TriggerType tt)
    {
        switch (tt)
        {
            case TriggerType::inletTemperature:
                return ReadingType::inletTemperature;
            case TriggerType::missingReadingsTimeout:
                throw errors::UnsupportedPolicyTriggerType();
                break;
            case TriggerType::gpio:
                return ReadingType::gpioState;
            case TriggerType::cpuUtilization:
                return ReadingType::cpuUtilization;
            case TriggerType::hostReset:
                return ReadingType::hostReset;
            case TriggerType::smbalertInterrupt:
                return ReadingType::smbalertInterrupt;
            default:
                throw errors::UnsupportedPolicyTriggerType();
        }
    }

  protected:
    std::shared_ptr<ActionIf> action;
    TriggerCallback callback;
};

} // namespace nodemanager