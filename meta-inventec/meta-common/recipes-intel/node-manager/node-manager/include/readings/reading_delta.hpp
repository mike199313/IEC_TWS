/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
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
class ReadingDelta : public Reading
{
  public:
    ReadingDelta(const ReadingDelta&) = delete;
    ReadingDelta& operator=(const ReadingDelta&) = delete;
    ReadingDelta(ReadingDelta&&) = delete;
    ReadingDelta& operator=(ReadingDelta&&) = delete;

    ReadingDelta(const std::shared_ptr<SensorReadingsManagerIf>&
                     sensorReadingsManagerArg,
                 ReadingType typeArg, unsigned int maxDeviceIndexArg,
                 double maxReadingValueArg) :
        Reading(sensorReadingsManagerArg, typeArg),
        maxDeviceIndex(maxDeviceIndexArg),
        lastReadingValues(maxDeviceIndexArg,
                          std::numeric_limits<double>::quiet_NaN()),
        maxReadingValue(maxReadingValueArg)
    {
    }

    virtual ~ReadingDelta() = default;

    void run() override
    {
        std::vector<double> deltas(maxDeviceIndex,
                                   std::numeric_limits<double>::quiet_NaN());
        bool isAnyValueValid = false;

        for (unsigned int index = 0; index < maxDeviceIndex; index++)
        {
            std::shared_ptr<SensorReadingIf> sensorReading =
                sensorReadingsManager->getAvailableAndValueValidSensorReading(
                    mapReadingTypeToSensorReadingType(readingType),
                    static_cast<DeviceIndex>(index));
            auto& previousValue = lastReadingValues.at(index);

            if (nullptr != sensorReading)
            {
                if (std::holds_alternative<double>(sensorReading->getValue()))
                {
                    auto currentValue =
                        std::get<double>(sensorReading->getValue());
                    if (!std::isnan(previousValue) && !std::isnan(currentValue))
                    {
                        isAnyValueValid = true;
                        if (currentValue < previousValue)
                        {
                            deltas.at(index) =
                                maxReadingValue + currentValue - previousValue;
                        }
                        else
                        {
                            deltas.at(index) = currentValue - previousValue;
                        }
                    }
                    previousValue = currentValue;
                }
                else
                {
                    throw std::logic_error("Unexpected reading type");
                }
            }
            else
            {
                previousValue = std::numeric_limits<double>::quiet_NaN();
            }
        }

        for (auto&& [readingConsumer, deviceIndex] : readingConsumers)
        {
            if (kAllDevices != deviceIndex)
            {
                updateConsumer(readingConsumer, deltas.at(deviceIndex),
                               deviceIndex);
            }
            else
            {
                auto value = std::numeric_limits<double>::quiet_NaN();
                if (isAnyValueValid)
                {
                    value = std::accumulate(deltas.begin(), deltas.end(), 0.0,
                                            [](double acc, double val) {
                                                if (!std::isnan(val))
                                                {
                                                    return acc + val;
                                                }
                                                return acc;
                                            });
                }
                updateConsumer(readingConsumer, value, deviceIndex);
            }
        }
    }

  private:
    unsigned int maxDeviceIndex;
    std::vector<double> lastReadingValues;
    double maxReadingValue;
};
} // namespace nodemanager
