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
#include "sensors/sensor_readings_manager.hpp"
#include "statistics/moving_average.hpp"
#include "utility/devices_configuration.hpp"

namespace nodemanager
{

static constexpr std::chrono::milliseconds kPsuAveragingTime{5000};

class ReadingPowerEfficiency : public Reading
{
  public:
    ReadingPowerEfficiency(const ReadingPowerEfficiency&) = delete;
    ReadingPowerEfficiency& operator=(const ReadingPowerEfficiency&) = delete;
    ReadingPowerEfficiency(ReadingPowerEfficiency&&) = delete;
    ReadingPowerEfficiency& operator=(ReadingPowerEfficiency&&) = delete;

    ReadingPowerEfficiency(const std::shared_ptr<SensorReadingsManagerIf>&
                               sensorReadingsManagerArg) :
        Reading(sensorReadingsManagerArg, ReadingType::platformPowerEfficiency),
        perDeviceReadingValues(
            kMaxPsuNumber, std::make_shared<MovingAverage>(kPsuAveragingTime))
    {
    }

    virtual ~ReadingPowerEfficiency() = default;

    // TBD: implement PSU redundancy awareness in the future
    void run() override final
    {
        double psuPowerEfficiency;
        double cumulatedPowerEfficiency =
            std::numeric_limits<double>::quiet_NaN();
        double acSum = 0.0;
        double dcSum = 0.0;
        bool isAnyReadingAvailAndValid = false;

        for (DeviceIndex psuIndex = 0; psuIndex < kMaxPsuNumber; ++psuIndex)
        {
            psuPowerEfficiency = std::numeric_limits<double>::quiet_NaN();

            auto sensorReadingAc =
                sensorReadingsManager->getAvailableAndValueValidSensorReading(
                    SensorReadingType::acPlatformPower, psuIndex);
            auto sensorReadingDc =
                sensorReadingsManager->getAvailableAndValueValidSensorReading(
                    SensorReadingType::dcPlatformPowerPsu, psuIndex);

            if (sensorReadingAc && sensorReadingDc)
            {

                if (std::holds_alternative<double>(
                        sensorReadingAc->getValue()) &&
                    std::holds_alternative<double>(sensorReadingDc->getValue()))
                {
                    isAnyReadingAvailAndValid = true;
                    acSum += std::get<double>(sensorReadingAc->getValue());
                    dcSum += std::get<double>(sensorReadingDc->getValue());

                    psuPowerEfficiency =
                        std::get<double>(sensorReadingDc->getValue()) /
                        std::get<double>(sensorReadingAc->getValue());
                    perDeviceReadingValues.at(psuIndex)->addSample(
                        psuPowerEfficiency);
                }
                else
                {
                    throw std::logic_error("Unexpected reading type");
                }
            }
        }

        if (isAnyReadingAvailAndValid)
        {
            cumulatedPowerEfficiency = dcSum / acSum;
            totalEfficiency->addSample(cumulatedPowerEfficiency);
        }

        for (auto&& reading : readingConsumers)
        {
            double value = std::numeric_limits<double>::quiet_NaN();
            const auto& deviceIndex = reading.second;

            if (deviceIndex < kMaxPsuNumber)
            {
                value = perDeviceReadingValues.at(deviceIndex)->getAvg();
            }
            else
            {
                value = totalEfficiency->getAvg();
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

  private:
    std::vector<std::shared_ptr<MovingAverage>> perDeviceReadingValues;
    std::shared_ptr<MovingAverage> totalEfficiency =
        std::make_shared<MovingAverage>(kPsuAveragingTime);
};
} // namespace nodemanager