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
class ReadingAverage : public Reading
{
  public:
    ReadingAverage(const ReadingAverage&) = delete;
    ReadingAverage& operator=(const ReadingAverage&) = delete;
    ReadingAverage(ReadingAverage&&) = delete;
    ReadingAverage& operator=(ReadingAverage&&) = delete;

    ReadingAverage(const std::shared_ptr<SensorReadingsManagerIf>&
                       sensorReadingsManagerArg,
                   ReadingType typeArg) :
        Reading(sensorReadingsManagerArg, typeArg)
    {
    }

    virtual ~ReadingAverage() = default;

    void run() override
    {
        for (const auto& [readingConsumer, deviceIndex] : readingConsumers)
        {
            T value = 0;
            uint32_t devicesNum = 0;

            sensorReadingsManager->forEachSensorReading(
                mapReadingTypeToSensorReadingType(readingType), deviceIndex,
                [&value, &devicesNum](const auto& sensorReading) {
                    if (sensorReading.isGood())
                    {
                        if (std::holds_alternative<T>(sensorReading.getValue()))
                        {
                            value += std::get<T>(sensorReading.getValue());
                            devicesNum++;
                        }
                        else
                        {
                            throw std::logic_error(
                                "[ReadingAverage] Unexpected reading type");
                        }
                    }
                });

            double avg = devicesNum == 0
                             ? std::numeric_limits<double>::quiet_NaN()
                             : static_cast<double>(value) / devicesNum;
            updateConsumer(readingConsumer, avg, deviceIndex);
        }
    }
};
} // namespace nodemanager
