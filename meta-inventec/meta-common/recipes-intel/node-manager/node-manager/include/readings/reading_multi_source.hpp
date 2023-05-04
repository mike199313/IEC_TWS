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
#include "utility/enum_to_string.hpp"

namespace nodemanager
{

class ReadingMultiSource : public Reading
{
  public:
    ReadingMultiSource(const ReadingMultiSource&) = delete;
    ReadingMultiSource& operator=(const ReadingMultiSource&) = delete;
    ReadingMultiSource(ReadingMultiSource&&) = delete;
    ReadingMultiSource& operator=(ReadingMultiSource&&) = delete;

    ReadingMultiSource(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        ReadingType readingTypeArg,
        const std::map<int, SensorReadingType>& readingSourcesArg) :
        Reading(sensorReadingsManagerArg, readingTypeArg),
        readingSources(readingSourcesArg), currentReadingSource(std::nullopt)
    {
    }

    virtual ~ReadingMultiSource() = default;

    void run() override final
    {
        double sum = std::numeric_limits<double>::quiet_NaN();
        bool isAnyReadingAvailAndValid;

        for (const auto& [priority, source] : readingSources)
        {
            isAnyReadingAvailAndValid = false;
            double value = 0.0;
            sensorReadingsManager->forEachSensorReading(
                source, kAllDevices,
                [&value, &isAnyReadingAvailAndValid,
                 this](const auto& sensorReading) {
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

            if (isAnyReadingAvailAndValid)
            {
                setReadingSource(source);
                sum = value;
                break;
            }
        }

        if (!isAnyReadingAvailAndValid)
        {
            setReadingSource(std::nullopt);
        }

        for (auto&& [consumer, deviceIndex] : readingConsumers)
        {
            updateConsumer(consumer, sum, deviceIndex);
        }
    }

    virtual std::optional<SensorReadingType> getReadingSource() const override
    {
        return currentReadingSource;
    }

    virtual void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer, ReadingType type,
        DeviceIndex deviceIndex = kAllDevices) override
    {
        if (deviceIndex == kAllDevices)
        {
            Reading::registerReadingConsumer(readingConsumer, type,
                                             deviceIndex);
        }
        else
        {
            throw std::logic_error("Unexpected deviceIndex");
        }
    }

  private:
    std::map<int, SensorReadingType> readingSources;
    std::optional<SensorReadingType> currentReadingSource;

    void setReadingSource(std::optional<SensorReadingType> newReadingSource)
    {
        if (newReadingSource != currentReadingSource)
        {
            currentReadingSource = newReadingSource;
            Logger::log<LogLevel::info>(
                "%s-%d reading source set to %s",
                enumToStr(kReadingTypeNames, readingType),
                unsigned{kAllDevices},
                currentReadingSource
                    ? enumToStr(kSensorReadingTypeNames, *currentReadingSource)
                    : "none");
            for (auto&& [consumer, deviceIndex] : readingConsumers)
            {
                consumer->reportEvent(ReadingEventType::readingSourceChanged,
                                      {readingType, deviceIndex});
            }
        }
    }
};

} // namespace nodemanager
