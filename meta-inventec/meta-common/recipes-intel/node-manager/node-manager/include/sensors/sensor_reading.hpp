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

#include "common_types.hpp"
#include "sensors/sensor_reading_if.hpp"
#include "utility/performance_monitor.hpp"

#include <boost/functional/hash.hpp>
#include <map>

namespace nodemanager
{
using SensorReadingEventCallback = std::function<void(
    SensorEventType eventType, std::shared_ptr<SensorReadingIf> sensorReading)>;

static const std::unordered_map<
    std::pair<SensorReadingStatus, SensorReadingStatus>,
    std::vector<SensorEventType>,
    boost::hash<std::pair<SensorReadingStatus, SensorReadingStatus>>>
    kTransitionEvents = {
        {{SensorReadingStatus::unavailable, SensorReadingStatus::invalid},
         {SensorEventType::sensorAppear}},
        {{SensorReadingStatus::unavailable, SensorReadingStatus::valid},
         {SensorEventType::sensorAppear, SensorEventType::readingAvailable}},
        {{SensorReadingStatus::invalid, SensorReadingStatus::unavailable},
         {SensorEventType::sensorDisappear}},
        {{SensorReadingStatus::invalid, SensorReadingStatus::valid},
         {SensorEventType::readingAvailable}},
        {{SensorReadingStatus::valid, SensorReadingStatus::unavailable},
         {SensorEventType::sensorDisappear, SensorEventType::readingMissing}},
        {{SensorReadingStatus::valid, SensorReadingStatus::invalid},
         {SensorEventType::readingMissing}},
};

class SensorReading : public SensorReadingIf,
                      public std::enable_shared_from_this<SensorReading>
{
  public:
    SensorReading(
        SensorReadingType sensorReadingTypeArg, DeviceIndex deviceIndexArg,
        const SensorReadingEventCallback& eventCallbackArg = nullptr) :
        sensorReadingType(sensorReadingTypeArg),
        deviceIndex(deviceIndexArg), eventCallback(eventCallbackArg)
    {
    }

    virtual ~SensorReading() = default;

    SensorReadingStatus getStatus() const override
    {
        if (status)
        {
            return *status;
        }
        return SensorReadingStatus::unavailable;
    }

    NmHealth getHealth() const override
    {
        return (getStatus() == SensorReadingStatus::invalid) ? NmHealth::warning
                                                             : NmHealth::ok;
    }

    bool isGood() const override
    {
        if (status)
        {
            return (SensorReadingStatus::valid == *status);
        }
        return false;
    }

    ValueType getValue() const override
    {
        return value;
    }

    void setStatus(const SensorReadingStatus newStatus)
    {
        if (status)
        {
            auto it =
                kTransitionEvents.find(std::make_pair(*status, newStatus));
            if (nullptr != eventCallback && it != kTransitionEvents.end())
            {
                for (SensorEventType event : it->second)
                {
                    eventCallback(event, shared_from_this());
                }
            }
        }
        else
        {
            generateEventsWhenStatusNotSet(newStatus);
        }

        status = newStatus;
    }

    void updateValue(ValueType newValue)
    {
        value = newValue;
    }

    SensorReadingType getSensorReadingType() const override
    {
        return sensorReadingType;
    }

    DeviceIndex getDeviceIndex() const override
    {
        return deviceIndex;
    }

  private:
    ValueType value{std::numeric_limits<double>::quiet_NaN()};
    SensorReadingType sensorReadingType;
    DeviceIndex deviceIndex;
    SensorReadingEventCallback eventCallback;
    std::optional<SensorReadingStatus> status = std::nullopt;

    void generateEventsWhenStatusNotSet(SensorReadingStatus newStatus)
    {
        if (nullptr == eventCallback)
        {
            return;
        }

        switch (newStatus)
        {
            case SensorReadingStatus::unavailable:
                eventCallback(SensorEventType::sensorDisappear,
                              shared_from_this());
                break;
            case SensorReadingStatus::invalid:
                eventCallback(SensorEventType::sensorAppear,
                              shared_from_this());
                break;
            case SensorReadingStatus::valid:
                eventCallback(SensorEventType::sensorAppear,
                              shared_from_this());
                eventCallback(SensorEventType::readingAvailable,
                              shared_from_this());
                break;
            default:
                throw std::logic_error("Unsupported sensor reading status");
        }
    }
};

} // namespace nodemanager