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
class ReadingHistoricalMax : public Reading
{
  public:
    ReadingHistoricalMax(const ReadingHistoricalMax&) = delete;
    ReadingHistoricalMax& operator=(const ReadingHistoricalMax&) = delete;
    ReadingHistoricalMax(ReadingHistoricalMax&&) = delete;
    ReadingHistoricalMax& operator=(ReadingHistoricalMax&&) = delete;

    ReadingHistoricalMax(const std::shared_ptr<SensorReadingsManagerIf>&
                             sensorReadingsManagerArg,
                         ReadingType typeArg, unsigned int maxDeviceIndexArg) :
        Reading(sensorReadingsManagerArg, typeArg),
        maxReadingValues(maxDeviceIndexArg,
                         std::numeric_limits<double>::quiet_NaN()),
        maxDeviceIndex(maxDeviceIndexArg)
    {
    }

    virtual ~ReadingHistoricalMax() = default;

    void run() override
    {
        static bool isAnyReadingAvailAndValid = false;
        for (unsigned int index = 0; index < maxDeviceIndex; index++)
        {
            std::shared_ptr<SensorReadingIf> sensorReading =
                sensorReadingsManager->getAvailableAndValueValidSensorReading(
                    mapReadingTypeToSensorReadingType(readingType),
                    static_cast<DeviceIndex>(index));

            if (nullptr != sensorReading)
            {
                if (std::holds_alternative<double>(sensorReading->getValue()))
                {
                    auto currentValue =
                        std::get<double>(sensorReading->getValue());
                    if (!std::isnan(currentValue))
                    {
                        isAnyReadingAvailAndValid = true;
                        maxReadingValues.at(index) =
                            std::max(currentValue, maxReadingValues.at(index));
                    }
                }
                else
                {
                    throw std::logic_error(
                        "[ReadingHistoricalMax] Unexpected reading type");
                }
            }
        }

        for (auto&& reading : readingConsumers)
        {
            const auto& deviceIndex = reading.second;

            if (kAllDevices != deviceIndex)
            {
                reading.first->updateValue(maxReadingValues.at(deviceIndex));
            }
            else
            {
                if (isAnyReadingAvailAndValid)
                {
                    auto value = std::accumulate(
                        maxReadingValues.begin(), maxReadingValues.end(), 0.0,
                        [](double sum, const double& curr) {
                            return sum + (std::isnan(curr) ? 0.0 : curr);
                        });
                    updateConsumer(reading.first, value, deviceIndex);
                }
                else
                {
                    updateConsumer(reading.first,
                                   std::numeric_limits<double>::quiet_NaN(),
                                   deviceIndex);
                }
            }
        }
    }

  private:
    std::vector<double> maxReadingValues;
    unsigned int maxDeviceIndex;
};
} // namespace nodemanager
