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

using Fraction = std::chrono::duration<double>;
using Percent =
    std::chrono::duration<double,
                          std::ratio_multiply<std::ratio<1, 1>, std::centi>>;

class ReadingCpuUtilization : public Reading
{
  public:
    ReadingCpuUtilization(const ReadingCpuUtilization&) = delete;
    ReadingCpuUtilization& operator=(const ReadingCpuUtilization&) = delete;
    ReadingCpuUtilization(ReadingCpuUtilization&&) = delete;
    ReadingCpuUtilization& operator=(ReadingCpuUtilization&&) = delete;

    ReadingCpuUtilization(const std::shared_ptr<SensorReadingsManagerIf>&
                              sensorReadingsManagerArg) :
        Reading(sensorReadingsManagerArg, ReadingType::cpuUtilization)
    {
    }

    virtual ~ReadingCpuUtilization() = default;

    void run() override
    {
        for (auto&& reading : readingConsumers)
        {
            double value = 0.0;
            uint64_t maxFreq = 0;
            bool isAnyReadingAvailAndValid = false;
            const auto& deviceIndex = reading.second;

            sensorReadingsManager->forEachSensorReading(
                mapReadingTypeToSensorReadingType(readingType), deviceIndex,
                [&value, &maxFreq,
                 &isAnyReadingAvailAndValid](const auto& sensorReading) {
                    if (sensorReading.isGood())
                    {
                        if (std::holds_alternative<CpuUtilizationType>(
                                sensorReading.getValue()))
                        {
                            auto c0 = std::get<CpuUtilizationType>(
                                sensorReading.getValue());
                            if (c0.duration.count() != 0)
                            {
                                maxFreq += c0.maxCpuUtilization;
                                value +=
                                    (static_cast<double>(c0.c0Delta) /
                                     static_cast<double>(c0.duration.count()));
                                isAnyReadingAvailAndValid = true;
                            }
                        }
                        else
                        {
                            throw std::logic_error("Unexpected reading type");
                        }
                    }
                });

            if (!isAnyReadingAvailAndValid || maxFreq == 0)
            {
                value = std::numeric_limits<double>::quiet_NaN();
            }
            else
            {
                Percent ret = std::chrono::duration_cast<Percent>(Fraction{
                    value / static_cast<double>(maxFreq)}); //[MHz/uSec %]
                value = ret.count();
            }
            updateConsumer(reading.first, value, deviceIndex);
        }
    }
};
} // namespace nodemanager
