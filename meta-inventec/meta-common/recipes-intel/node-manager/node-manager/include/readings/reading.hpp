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

#include "common_types.hpp"
#include "flow_control.hpp"
#include "reading_consumer.hpp"
#include "reading_type.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "sensors/sensor_readings_manager.hpp"

namespace nodemanager
{

static const std::chrono::seconds kReadingAvailabilityTimeout{20};

class ReadingIf : public ReadingEventDispatcherIf, public RunnerIf
{
  public:
    virtual ~ReadingIf() = default;

    virtual ReadingType getReadingType() const = 0;
    virtual std::optional<SensorReadingType> getReadingSource() const = 0;
};

class Reading : public ReadingIf
{
  public:
    Reading() = delete;
    Reading(const Reading&) = delete;
    Reading& operator=(const Reading&) = delete;
    Reading(Reading&&) = delete;
    Reading& operator=(Reading&&) = delete;

    Reading(const std::shared_ptr<SensorReadingsManagerIf>&
                sensorReadingsManagerArg,
            ReadingType typeArg) :
        readingType(typeArg),
        sensorReadingsManager(sensorReadingsManagerArg)
    {
        initTimestamp = Clock::now();
    }

    virtual ~Reading()
    {
    }

    void run() override
    {
        for (auto&& reading : readingConsumers)
        {
            double value = 0.0;
            bool isAnyReadingAvailAndValid = false;
            const auto& deviceIndex = reading.second;

            sensorReadingsManager->forEachSensorReading(
                mapReadingTypeToSensorReadingType(readingType), deviceIndex,
                [&value,
                 &isAnyReadingAvailAndValid](const auto& sensorReading) {
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
            updateConsumer(reading.first, value, deviceIndex);
        }
    }

    ReadingType getReadingType() const
    {
        return readingType;
    }

    virtual std::optional<SensorReadingType> getReadingSource() const
    {
        return mapReadingTypeToSensorReadingType(readingType);
    }

    virtual void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer, ReadingType type,
        DeviceIndex deviceIndex = kAllDevices) override
    {
        sensorReadingsManager->registerReadingConsumer(readingConsumer, type,
                                                       deviceIndex);
        readingConsumers[readingConsumer] = deviceIndex;
        lastReportedReadings[readingConsumer] =
            ReadingEventType::readingUnavailable;
    }

    virtual void unregisterReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer) override
    {
        sensorReadingsManager->unregisterReadingConsumer(readingConsumer);
        readingConsumers.erase(readingConsumer);
        lastReportedReadings.erase(readingConsumer);
        firstEventSent.erase(readingConsumer);
    }

  protected:
    ReadingType readingType;
    std::unordered_map<std::shared_ptr<ReadingConsumer>, DeviceIndex>
        readingConsumers;
    std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager;
    std::unordered_map<std::shared_ptr<ReadingConsumer>, ReadingEventType>
        lastReportedReadings;

    void updateConsumer(std::shared_ptr<ReadingConsumer> readingConsumer,
                        double value, DeviceIndex idx)
    {
        readingConsumer->updateValue(value);
        ReadingEventType event = std::isnan(value)
                                     ? ReadingEventType::readingUnavailable
                                     : ReadingEventType::readingAvailable;
        if (event != lastReportedReadings[readingConsumer] ||
            (!firstEventSent[readingConsumer] &&
             (event == ReadingEventType::readingUnavailable) &&
             ((Clock::now() - initTimestamp) > kReadingAvailabilityTimeout)))
        {
            readingConsumer->reportEvent(event, {readingType, idx});
            firstEventSent[readingConsumer] = true;
        }
        lastReportedReadings[readingConsumer] = event;
    }

  private:
    Clock::time_point initTimestamp;
    std::unordered_map<std::shared_ptr<ReadingConsumer>, bool> firstEventSent;
};

} // namespace nodemanager
