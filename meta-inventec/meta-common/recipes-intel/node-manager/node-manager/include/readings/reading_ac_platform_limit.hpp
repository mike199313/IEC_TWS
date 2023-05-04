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
#include "readings/reading_event.hpp"
#include "utility/devices_configuration.hpp"

namespace nodemanager
{

class ReadingAcPlatformLimit : public Reading
{
  public:
    ReadingAcPlatformLimit(const ReadingAcPlatformLimit&) = delete;
    ReadingAcPlatformLimit& operator=(const ReadingAcPlatformLimit&) = delete;
    ReadingAcPlatformLimit(ReadingAcPlatformLimit&&) = delete;
    ReadingAcPlatformLimit& operator=(ReadingAcPlatformLimit&&) = delete;

    ReadingAcPlatformLimit(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        std::shared_ptr<ReadingEventDispatcherIf> efficiencyReadingArg) :
        Reading(sensorReadingsManagerArg, ReadingType::acPlatformPowerLimit),
        efficiencyReading(efficiencyReadingArg)
    {
        efficiencyReading->registerReadingConsumer(
            psuEfficiencyHandler, ReadingType::platformPowerEfficiency,
            kAllDevices);
    }

    virtual ~ReadingAcPlatformLimit()
    {
        efficiencyReading->unregisterReadingConsumer(psuEfficiencyHandler);
    }

    void run() override
    {
        double value = 0;
        auto isAnyReadingAvailAndValid = false;

        sensorReadingsManager->forEachSensorReading(
            SensorReadingType::dcPlatformPowerLimit, 0,
            [&value, &isAnyReadingAvailAndValid](auto& sensorReading) {
                if (sensorReading.isGood())
                {
                    if (std::holds_alternative<double>(
                            sensorReading.getValue()))
                    {
                        isAnyReadingAvailAndValid = true;
                        value += std::get<double>(sensorReading.getValue());
                    }
                    else
                    {
                        throw std::logic_error("Unexpected reading type");
                    }
                }
            });
        if (!isAnyReadingAvailAndValid)
        {
            value = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
            value /= psuEfficiency;
        }

        for (auto&& [readingConsumer, deviceIndex] : readingConsumers)
        {
            updateConsumer(readingConsumer, value, deviceIndex);
        }
    }

  private:
    std::shared_ptr<ReadingEventDispatcherIf> efficiencyReading;
    double psuEfficiency = std::numeric_limits<double>::quiet_NaN();

    std::shared_ptr<ReadingEvent> psuEfficiencyHandler =
        std::make_shared<ReadingEvent>(
            [this](double incomingValue) { psuEfficiency = incomingValue; });
};

} // namespace nodemanager
