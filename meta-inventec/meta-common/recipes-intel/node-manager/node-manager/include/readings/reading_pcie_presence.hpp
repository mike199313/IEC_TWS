/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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
class ReadingPciePresence : public Reading
{
  public:
    ReadingPciePresence(const ReadingPciePresence&) = delete;
    ReadingPciePresence& operator=(const ReadingPciePresence&) = delete;
    ReadingPciePresence(ReadingPciePresence&&) = delete;
    ReadingPciePresence& operator=(ReadingPciePresence&&) = delete;

    ReadingPciePresence(const std::shared_ptr<SensorReadingsManagerIf>&
                            sensorReadingsManagerArg) :
        Reading(sensorReadingsManagerArg, ReadingType::pciePresence)
    {
    }

    virtual ~ReadingPciePresence() = default;

    void run() override final
    {
        std::bitset<kMaxPcieNumber> pciPresenceMap;
        sensorReadingsManager->forEachSensorReading(
            mapReadingTypeToSensorReadingType(ReadingType::pciePower),
            kAllDevices, [&pciPresenceMap](auto& sensorReading) {
                if (sensorReading.getDeviceIndex() >= kMaxPcieNumber)
                {
                    throw std::logic_error(
                        std::string("Peci DeviceIndex out of range: ") +
                        std::to_string(sensorReading.getDeviceIndex()) + ">" +
                        std::to_string(kMaxPcieNumber));
                }
                if (sensorReading.getStatus() !=
                    SensorReadingStatus::unavailable)
                {
                    pciPresenceMap.set(sensorReading.getDeviceIndex());
                }
            });

        for (auto&& [consumer, deviceIndex] : readingConsumers)
        {
            double value = std::numeric_limits<double>::quiet_NaN();
            if (deviceIndex == kAllDevices)
            {
                value = static_cast<double>(pciPresenceMap.to_ulong());
            }
            updateConsumer(consumer, value, deviceIndex);
        }
    }

    void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer,
        [[maybe_unused]] ReadingType type,
        DeviceIndex deviceIndex = kAllDevices) final
    {
        readingConsumers[readingConsumer] = deviceIndex;
    }

    void unregisterReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer) final
    {
        readingConsumers.erase(readingConsumer);
    }
};
} // namespace nodemanager
