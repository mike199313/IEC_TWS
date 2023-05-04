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
#include "utility/devices_configuration.hpp"

#include <bitset>

namespace nodemanager
{
template <class T>
class ReadingMin : public Reading
{
  public:
    ReadingMin(const ReadingMin&) = delete;
    ReadingMin& operator=(const ReadingMin&) = delete;
    ReadingMin(ReadingMin&&) = delete;
    ReadingMin& operator=(ReadingMin&&) = delete;

    ReadingMin(const std::shared_ptr<SensorReadingsManagerIf>&
                   sensorReadingsManagerArg,
               ReadingType typeArg) :
        Reading(sensorReadingsManagerArg, typeArg)
    {
    }

    virtual ~ReadingMin() = default;

    void run() override final
    {
        bool isAnyValue = false;
        T min = std::numeric_limits<T>::max();

        sensorReadingsManager->forEachSensorReading(
            mapReadingTypeToSensorReadingType(getReadingType()), kAllDevices,
            [&min, &isAnyValue](auto& sensorReading) {
                if (sensorReading.isGood())
                {
                    if (std::holds_alternative<T>(sensorReading.getValue()))
                    {
                        min = std::min(min,
                                       std::get<T>(sensorReading.getValue()));
                        isAnyValue = true;
                    }
                    else
                    {
                        throw std::logic_error("Unexpected reading type");
                    }
                }
            });

        for (auto&& reading : readingConsumers)
        {
            const auto& deviceIndex = reading.second;
            double value = std::numeric_limits<double>::quiet_NaN();
            if (isAnyValue && deviceIndex == kAllDevices)
            {
                value = static_cast<double>(min);
            }
            updateConsumer(reading.first, value, deviceIndex);
        }
    }
};
} // namespace nodemanager
