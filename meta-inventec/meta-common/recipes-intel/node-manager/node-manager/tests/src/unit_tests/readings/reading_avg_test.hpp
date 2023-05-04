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

#include "mocks/reading_consumer_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "reading_base.hpp"
#include "readings/reading_average.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nodemanager
{

class ReadingAvgTest : public ReadingTestBase, public ::testing::Test
{
  public:
    ReadingAvgTest() :
        ReadingTestBase(ReadingType::cpuAverageFrequency,
                        SensorReadingType::cpuAverageFrequency)
    {
    }

    virtual void SetUp() override
    {
        sut_ = std::make_shared<ReadingAverage<double>>(sensorReadingsManager_,
                                                        readingType);

        unsigned int index;
        for (index = 0; index < kAllDevicesNum; index++)
        {
            sensorReadings_.push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());
            readingConsumers_.push_back(
                std::make_shared<testing::NiceMock<ReadingConsumerMock>>());

            sut_->registerReadingConsumer(readingConsumers_.at(index),
                                          readingType, index);
        }

        // one more for kAllDevices reading
        readingConsumers_.push_back(
            std::make_shared<testing::NiceMock<ReadingConsumerMock>>());
        sut_->registerReadingConsumer(readingConsumers_.at(kAllDevicesNum),
                                      readingType, kAllDevices);

        ON_CALL(
            *sensorReadingsManager_,
            forEachSensorReading(sensorReadingType, kAllDevices, testing::_))
            .WillByDefault(testing::Invoke(
                [this](auto type, auto deviceIndex, auto action) {
                    for (auto sensor : sensorReadings_)
                    {
                        action(*(sensor));
                    }
                    return true;
                }));
    }
};

TEST_F(ReadingAvgTest, NoUpdateFromDevicesExpectNansReported)
{
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN()}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDevicesIntegerValuesOutputFromEachDeviceSame)
{
    setupSensorReadings({{true, 800}, {true, 800}});
    setupExpectedReadings({{800, 800, (800 + 800) / 2}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDevicesIntegerValuesOutputFromEachDeviceDiff)
{
    setupSensorReadings({{true, 1000}, {true, 2500}});
    setupExpectedReadings({{1000, 2500, (1000 + 2500) / 2}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDeviceFloatingValuesOutputFromEachDeviceSame)
{
    setupSensorReadings({{true, 800.25}, {true, 800.25}});
    setupExpectedReadings({{800.25, 800.25, (800.25 + 800.25) / 2.0}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDevicesFloatingValuesOutputFromEachDeviceDiff)
{
    setupSensorReadings({{true, 1000.15}, {true, 2500.75}});
    setupExpectedReadings({{1000.15, 2500.75, (1000.15 + 2500.75) / 2.0}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDevicesFirstReadingNotValid)
{
    setupSensorReadings({{false, 800}, {true, 1000}});
    setupExpectedReadings(
        {{std::numeric_limits<double>::quiet_NaN(), 1000, 1000}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDevicesSecondReadingNotValid)
{
    setupSensorReadings({{true, 800}, {false, 1000}});
    setupExpectedReadings(
        {{800, std::numeric_limits<double>::quiet_NaN(), 800}});
    sut_->run();
}

TEST_F(ReadingAvgTest, SingleDeviceLimitedOutputProperlyHandled)
{
    setupSensorReadings({{true, std::numeric_limits<double>::max()}});
    setupExpectedReadings({{std::numeric_limits<double>::max(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::max()}});
    sut_->run();
}

TEST_F(ReadingAvgTest, TwoDevicesLimitedOutputsProperlyHandled)
{
    setupSensorReadings({{true, std::numeric_limits<double>::max()},
                         {true, std::numeric_limits<double>::max()}});
    setupExpectedReadings({{std::numeric_limits<double>::max(),
                            std::numeric_limits<double>::max(),
                            std::numeric_limits<double>::infinity()}});
    sut_->run();
}

} // namespace nodemanager
