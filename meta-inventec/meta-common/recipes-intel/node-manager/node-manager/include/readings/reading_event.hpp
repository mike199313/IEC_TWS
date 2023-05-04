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

#include "reading_consumer.hpp"
#include "reading_type.hpp"

#include <functional>

namespace nodemanager
{

using ReadingCallback = std::function<void(double)>;
using EventCallback = std::function<void(SensorEventType eventType,
                                         const SensorContext& sensorCtx,
                                         const ReadingContext& readingCtx)>;
using ReadingEventCallback = std::function<void(
    const ReadingEventType eventType, const ReadingContext& readingCtx)>;

class ReadingEvent : public ReadingConsumer
{
  public:
    ReadingEvent() = delete;
    ReadingEvent(const ReadingEvent&) = delete;
    ReadingEvent& operator=(const ReadingEvent&) = delete;
    ReadingEvent(ReadingEvent&&) = delete;
    ReadingEvent& operator=(ReadingEvent&&) = delete;

    ReadingEvent(const ReadingCallback readingCallbackArg,
                 const EventCallback eventCallbackArg = nullptr,
                 const ReadingEventCallback readingEventCallbackArg = nullptr) :
        readingCallback(readingCallbackArg),
        eventCallback(eventCallbackArg),
        readingEventCallback(readingEventCallbackArg)
    {
    }

    virtual ~ReadingEvent() = default;

    void updateValue(double valueArg) override
    {
        if (readingCallback != nullptr)
        {
            readingCallback(valueArg);
        }
    }

    void reportEvent(SensorEventType eventType, const SensorContext& sensorCtx,
                     const ReadingContext& readingCtx) override
    {
        if (eventCallback != nullptr)
        {
            eventCallback(eventType, sensorCtx, readingCtx);
        }
    }

    void reportEvent(const ReadingEventType eventType,
                     const ReadingContext& readingCtx)
    {
        if (readingEventCallback != nullptr)
        {
            readingEventCallback(eventType, readingCtx);
        }
    }

  private:
    ReadingCallback readingCallback;
    EventCallback eventCallback;
    ReadingEventCallback readingEventCallback;
};

} // namespace nodemanager
