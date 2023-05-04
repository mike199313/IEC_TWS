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

#include "reading.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "utility/devices_configuration.hpp"

#include <bitset>

namespace nodemanager
{
class ReadingSmbalertInterrupt : public Reading
{
  public:
    ReadingSmbalertInterrupt(const ReadingSmbalertInterrupt&) = delete;
    ReadingSmbalertInterrupt&
        operator=(const ReadingSmbalertInterrupt&) = delete;
    ReadingSmbalertInterrupt(ReadingSmbalertInterrupt&&) = delete;
    ReadingSmbalertInterrupt& operator=(ReadingSmbalertInterrupt&&) = delete;

    ReadingSmbalertInterrupt(const std::shared_ptr<SensorReadingsManagerIf>&
                                 sensorReadingsManagerArg) :
        Reading(sensorReadingsManagerArg, ReadingType::smbalertInterrupt)
    {
    }

    virtual ~ReadingSmbalertInterrupt() = default;

    void run() override final
    {
        double interrupt = 0;

        auto sensor = sensorReadingsManager->getSensorReading(
            SensorReadingType::smartStatus, kSmartDeviceIndex);

        if (sensor && sensor->isGood())
        {
            if (std::get<SmartStatusType>(sensor->getValue()) ==
                SmartStatusType::interruptHandling)
            {
                interrupt = 1;
            }
        }

        for (auto&& reading : readingConsumers)
        {
            const auto& deviceIndex = reading.second;
            updateConsumer(reading.first, interrupt, deviceIndex);
        }
    }

    virtual void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer,
        [[maybe_unused]] ReadingType type,
        DeviceIndex deviceIndex = kAllDevices) final
    {
        readingConsumers[readingConsumer] = deviceIndex;
    }

    virtual void unregisterReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer) final
    {
        readingConsumers.erase(readingConsumer);
    }
};
} // namespace nodemanager
