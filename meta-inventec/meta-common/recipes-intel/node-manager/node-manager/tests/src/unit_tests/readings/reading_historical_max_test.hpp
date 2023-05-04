/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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
#include "readings/reading_historical_max.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nodemanager
{

class ReadingHistoricalMaxTest : public ReadingTestBase, public ::testing::Test
{
  public:
    ReadingHistoricalMaxTest() :
        ReadingTestBase(ReadingType::pciePowerCapabilitiesMax,
                        mapReadingTypeToSensorReadingType(
                            ReadingType::pciePowerCapabilitiesMax))
    {
    }

    virtual void SetUp() override
    {
        sut_ = std::make_shared<ReadingHistoricalMax>(
            sensorReadingsManager_, readingType, kAllDevicesNum);

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
    }
};

TEST_F(ReadingHistoricalMaxTest, NoUpdateExpectNansReported)
{
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN()}});
    sut_->run();
}

TEST_F(ReadingHistoricalMaxTest,
       UpdateSingleSensorReadingExpectCorrectReadingsReported)
{
    setupSensorReadings({{true, 1}});
    setupExpectedReadings({{1, std::numeric_limits<double>::quiet_NaN(), 1}});
    sut_->run();
}

TEST_F(ReadingHistoricalMaxTest,
       UpdateSensorReadingsOnceExpectCorrectMaxReadingsReported)
{
    setupExpectedReadings({{2, 3, 2 + 3}});
    setupSensorReadings({{true, 2}, {true, 3}});
    sut_->run();
}

TEST_F(ReadingHistoricalMaxTest,
       UpdateSensorReadingsTwiceExpectCorrectMaxReadingsReported)
{
    setupSensorReadings({{true, 1}, {true, 2}});
    sut_->run();

    setupExpectedReadings({{4, 5, 4 + 5}});
    setupSensorReadings({{true, 4}, {true, 5}});
    sut_->run();
}

TEST_F(ReadingHistoricalMaxTest,
       NoSensorReadingsAvailableAfterPreviousUpdatesExpectPreviousMaxReported)
{
    setupSensorReadings({{true, 6}, {true, 7}});
    sut_->run();

    setupExpectedReadings({{6, 7, 6 + 7}});
    setupSensorReadings({{false, std::numeric_limits<double>::quiet_NaN()},
                         {false, std::numeric_limits<double>::quiet_NaN()}});
    sut_->run();
}

TEST_F(ReadingHistoricalMaxTest,
       UpdateSensorReadingsWithDecreasingValuesExpectCorrectMaxReported)
{
    setupSensorReadings({{true, 8}, {true, 9}});
    sut_->run();

    setupSensorReadings({{true, 7}, {true, 6}});
    sut_->run();

    setupExpectedReadings({{8, 9, 8 + 9}});
    setupSensorReadings({{true, 5}, {true, 4}});
    sut_->run();
}

} // namespace nodemanager