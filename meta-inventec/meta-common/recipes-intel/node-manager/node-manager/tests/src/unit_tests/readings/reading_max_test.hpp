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
#include "reading_base.hpp"
#include "readings/reading_max.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nodemanager
{

class ReadingMaxTest : public ReadingTestBase, public ::testing::Test
{
  public:
    ReadingMaxTest() :
        ReadingTestBase(ReadingType::prochotRatioCapabilitiesMin,
                        SensorReadingType::prochotRatioCapabilitiesMin)
    {
    }

    virtual void SetUp() override
    {
        sut_ = std::make_shared<ReadingMax<double>>(sensorReadingsManager_,
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

TEST_F(ReadingMaxTest, NoUpdateExpectNansReported)
{
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN()}});
    sut_->run();
}

TEST_F(ReadingMaxTest, ForSinglSensorMaxIsSensorValue)
{
    setupSensorReadings({{true, 1}});
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(), 1}});
    sut_->run();
}

TEST_F(ReadingMaxTest, TwoSensorsTheFistHasHigherValueAndThisIsMax)
{
    setupSensorReadings({{true, 20}, {true, 10}});
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(), 20}});
    sut_->run();
}

TEST_F(ReadingMaxTest, TwoSensorsTheSecondHasHigherValueAndThisIsMax)
{
    setupSensorReadings({{true, 10}, {true, 20}});
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(), 20}});
    sut_->run();
}

TEST_F(ReadingMaxTest, BothSensorsEqual)
{
    setupSensorReadings({{true, 10}, {true, 10}});
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(), 10}});
    sut_->run();
}

TEST_F(ReadingMaxTest, LimitsAreAlsoSupported)
{
    setupSensorReadings({{true, std::numeric_limits<double>::max()}});
    setupExpectedReadings({{std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::quiet_NaN(),
                            std::numeric_limits<double>::max()}});
    sut_->run();
}

} // namespace nodemanager