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
#include "readings/reading_delta.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nodemanager
{
static constexpr double kMaxReadingValue = 10.5;

class ReadingDeltaTest : public ReadingTestBase, public ::testing::Test
{
  public:
    ReadingDeltaTest() :
        ReadingTestBase(ReadingType::cpuEnergy, SensorReadingType::cpuEnergy)
    {
    }

    virtual void SetUp() override
    {
        sut_ =
            std::make_shared<ReadingDelta>(sensorReadingsManager_, readingType,
                                           kAllDevicesNum, kMaxReadingValue);

        unsigned int index;
        for (index = 0; index < kAllDevicesNum; index++)
        {
            readingConsumers_.push_back(
                std::make_shared<testing::NiceMock<ReadingConsumerMock>>());
            sensorReadings_.push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());
            sut_->registerReadingConsumer(readingConsumers_.at(index),
                                          readingType,
                                          static_cast<DeviceIndex>(index));
        }

        readingConsumers_.push_back(
            std::make_shared<testing::NiceMock<ReadingConsumerMock>>());
        sut_->registerReadingConsumer(readingConsumers_.at(index),
                                      ReadingType::cpuEnergy, kAllDevices);
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
                            SensorReadingType::cpuEnergy,
                            static_cast<DeviceIndex>(index)))
                    .WillByDefault(testing::Return(sensorReadings_.at(index)));
                ON_CALL(*sensorReadings_.at(index), getValue())
                    .WillByDefault(testing::Return(config.value));
            }
            else
            {
                ON_CALL(*sensorReadingsManager_,
                        getAvailableAndValueValidSensorReading(
                            SensorReadingType::cpuEnergy,
                            static_cast<DeviceIndex>(index)))
                    .WillByDefault(testing::Return(nullptr));
            }
            index++;
        }
    }

    void setupExpectedDeltas(std::vector<std::vector<double>> deltas)
    {
        for (auto runLoop : deltas)
        {
            unsigned int index = 0;
            for (auto delta : runLoop)
            {
                EXPECT_CALL(*readingConsumers_.at(index),
                            updateValue(testing::NanSensitiveDoubleEq(delta)));
                index++;
            }
        }
    }
};

TEST_F(ReadingDeltaTest, SendFirstSampleExpectNanDelta)
{
    setupExpectedDeltas({{std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN()}});

    setupSensorReadings({{true, 1.23}, {true, 4.56}});
    sut_->run();
}

TEST_F(ReadingDeltaTest, SendTwoSamplesExpectCorretcDelta)
{
    setupSensorReadings({{true, 1.23}, {true, 4.56}});
    sut_->run();

    setupExpectedReadings(
        {{9.87 - 1.23, 6.54 - 4.56, (9.87 - 1.23) + (6.54 - 4.56)}});

    setupSensorReadings({{true, 9.87}, {true, 6.54}});
    sut_->run();
}

TEST_F(ReadingDeltaTest, SendNonNanValueAndThenNanExpectNanDelta)
{
    setupSensorReadings({{true, 1.23}, {true, 4.56}});
    sut_->run();

    setupSensorReadings({{true, 2.45}, {true, 5.67}});
    sut_->run();

    setupExpectedDeltas({{std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN()}});

    setupSensorReadings({{true, std::numeric_limits<double>::quiet_NaN()},
                         {true, std::numeric_limits<double>::quiet_NaN()}});
    sut_->run();
}

TEST_F(ReadingDeltaTest, SendNonNanValuesAfterNanExpectCorrectDelta)
{
    setupSensorReadings({{true, 1.23}, {true, 4.56}});
    sut_->run();

    setupSensorReadings({{true, 2.45}, {true, 5.67}});
    sut_->run();

    setupSensorReadings({{true, std::numeric_limits<double>::quiet_NaN()},
                         {true, std::numeric_limits<double>::quiet_NaN()}});

    setupSensorReadings({{true, 3.45}, {true, 6.78}});
    sut_->run();

    setupExpectedReadings(
        {{4.56 - 3.45, 7.89 - 6.78, (4.56 - 3.45) + (7.89 - 6.78)}});

    setupSensorReadings({{true, 4.56}, {true, 7.89}});
    sut_->run();
}

TEST_F(ReadingDeltaTest,
       SendLowerValueThanPreviousOneExpectCorrectDeltaWithOverflowHandled)
{
    setupSensorReadings({{true, 1.23}, {true, 4.56}});
    sut_->run();

    setupExpectedReadings(
        {{kMaxReadingValue - (1.23 - 0.12), kMaxReadingValue - (4.56 - 2.34),
          (kMaxReadingValue - (1.23 - 0.12)) +
              (kMaxReadingValue - (4.56 - 2.34))}});

    setupSensorReadings({{true, 0.12}, {true, 2.34}});
    sut_->run();
}

TEST_F(ReadingDeltaTest, NoSensorReadingAvailableExpectNanDelta)
{
    setupSensorReadings({{true, 1.23}, {true, 4.56}});
    sut_->run();

    setupSensorReadings({{true, 4.56}, {true, 7.89}});
    sut_->run();

    setupExpectedDeltas({{std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::quiet_NaN()}});

    setupSensorReadings({{false, 0.12}, {false, 2.34}});
    sut_->run();
}

} // namespace nodemanager