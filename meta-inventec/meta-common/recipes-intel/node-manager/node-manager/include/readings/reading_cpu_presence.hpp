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

#include "reading.hpp"
#include "utility/devices_configuration.hpp"

#include <bitset>

namespace nodemanager
{
class ReadingCpuPresence : public Reading
{
  public:
    ReadingCpuPresence(const ReadingCpuPresence&) = delete;
    ReadingCpuPresence& operator=(const ReadingCpuPresence&) = delete;
    ReadingCpuPresence(ReadingCpuPresence&&) = delete;
    ReadingCpuPresence& operator=(ReadingCpuPresence&&) = delete;

    ReadingCpuPresence(const std::shared_ptr<SensorReadingsManagerIf>&
                           sensorReadingsManagerArg) :
        Reading(sensorReadingsManagerArg, ReadingType::cpuPresence)
    {
    }

    virtual ~ReadingCpuPresence() = default;

    void run() override final
    {
        std::bitset<kMaxCpuNumber> cpuPresenceMap;

        sensorReadingsManager->forEachSensorReading(
            SensorReadingType::cpuPackagePower, kAllDevices,
            [&cpuPresenceMap](auto& sensorReading) {
                if (sensorReading.getDeviceIndex() >= kMaxCpuNumber)
                {
                    throw std::logic_error(
                        std::string("CPU DeviceIndex out of range: ") +
                        std::to_string(sensorReading.getDeviceIndex()) + ">" +
                        std::to_string(kMaxCpuNumber));
                }
                if (sensorReading.getStatus() !=
                    SensorReadingStatus::unavailable)
                {
                    cpuPresenceMap.set(sensorReading.getDeviceIndex());
                }
            });

        for (auto&& reading : readingConsumers)
        {
            double value = std::numeric_limits<double>::quiet_NaN();
            const auto& deviceIndex = reading.second;

            if (deviceIndex == kAllDevices)
            {
                value = static_cast<double>(cpuPresenceMap.to_ulong());
            }

            updateConsumer(reading.first, value, deviceIndex);
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
