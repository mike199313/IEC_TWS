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
#include "reading_type.hpp"

#include <memory>

namespace nodemanager
{

struct SensorContext
{
    SensorReadingType type;
    DeviceIndex deviceIndex;
};

struct ReadingContext
{
    ReadingType type;
    DeviceIndex deviceIndex;
};

class ReadingConsumer
{
  public:
    virtual ~ReadingConsumer() = default;
    virtual void updateValue(double value) = 0;
    virtual void reportEvent(SensorEventType eventType,
                             const SensorContext& sensorCtx,
                             const ReadingContext& readingCtx) = 0;
    virtual void reportEvent(const ReadingEventType eventType,
                             const ReadingContext& readingCtx) = 0;
};

class ReadingEventDispatcherIf
{
  public:
    virtual ~ReadingEventDispatcherIf() = default;
    virtual void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer,
        ReadingType readingType, DeviceIndex deviceIndex = kAllDevices) = 0;

    virtual void unregisterReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer) = 0;
};

} // namespace nodemanager
