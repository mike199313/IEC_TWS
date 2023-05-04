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

#include "mocks/reading_consumer_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "readings/reading.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nodemanager
{
static constexpr unsigned int kAllDevicesNum = 2;

struct SensorReadingConfiguration
{
    bool isValidAndAvailable;
    double value;
};

class ReadingTestBase
{
  public:
    ReadingTestBase(const ReadingTestBase&) = delete;
    ReadingTestBase& operator=(const ReadingTestBase&) = delete;
    ReadingTestBase(ReadingTestBase&&) = delete;
    ReadingTestBase& operator=(ReadingTestBase&&) = delete;

    ReadingTestBase(ReadingType readingTypeArg,
                    SensorReadingType sensorReadingTypeArg) :
        readingType(readingTypeArg),
        sensorReadingType(sensorReadingTypeArg)
    {
    }

    void setupSensorReadings(
        std::vector<SensorReadingConfiguration> sensorsConfig)
    {
        unsigned int index = 0;
        for (auto config : sensorsConfig)
        {
            if (config.isValidAndAvailable)
            {
                ON_CALL(*sensorReadingsManager_,
                        getAvailableAndValueValidSensorReading(
                            sensorReadingType, static_cast<DeviceIndex>(index)))
                    .WillByDefault(testing::Return(sensorReadings_.at(index)));
                ON_CALL(*sensorReadings_.at(index), getValue())
                    .WillByDefault(testing::Return(config.value));
                ON_CALL(*sensorReadings_.at(index), isGood())
                    .WillByDefault(testing::Return(config.isValidAndAvailable));

                ON_CALL(*sensorReadingsManager_,
                        forEachSensorReading(sensorReadingType,
                                             static_cast<DeviceIndex>(index),
                                             testing::_))
                    .WillByDefault(testing::Invoke(
                        [this, index](auto type, auto deviceIndex,
                                      auto action) {
                            action(*(sensorReadings_.at(index)));
                            return true;
                        }));
            }
            else
            {
                ON_CALL(*sensorReadingsManager_,
                        getAvailableAndValueValidSensorReading(
                            sensorReadingType, static_cast<DeviceIndex>(index)))
                    .WillByDefault(testing::Return(nullptr));
            }
            index++;
        }
    }

    void setupExpectedReadings(std::vector<std::vector<double>> values)
    {
        for (auto runLoop : values)
        {
            unsigned int index = 0;
            for (auto value : runLoop)
            {
                EXPECT_CALL(*readingConsumers_.at(index),
                            updateValue(testing::NanSensitiveDoubleEq(value)));
                index++;
            }
        }
    }

  protected:
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::vector<std::shared_ptr<ReadingConsumerMock>> readingConsumers_;
    std::vector<std::shared_ptr<SensorReadingMock>> sensorReadings_;
    ReadingType readingType;
    SensorReadingType sensorReadingType;
    std::shared_ptr<Reading> sut_;
};

} // namespace nodemanager