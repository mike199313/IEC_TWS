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
#include "mocks/reading_consumer_mock.hpp"
#include "mocks/reading_event_dispatcher_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "readings/reading_ac_platform_limit.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class ReadingAcPowerLimitTest : public ::testing::Test
{
  protected:
    std::shared_ptr<ReadingAcPlatformLimit> sut_;
    std::shared_ptr<ReadingEventDispatcherMock> powerEfficiencyReading =
        std::make_shared<testing::NiceMock<ReadingEventDispatcherMock>>();
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<SensorReadingMock> sensorPowerLimitReading =
        std::make_shared<testing::NiceMock<SensorReadingMock>>();
    std::shared_ptr<ReadingConsumerMock> readingConsumer =
        std::make_shared<testing::NiceMock<ReadingConsumerMock>>();
    std::shared_ptr<ReadingConsumer> efficiencyReadingConsumer = nullptr;

    void SetUp() override
    {
        ON_CALL(*sensorPowerLimitReading, getValue)
            .WillByDefault(testing::Return(200.0));
        ON_CALL(*sensorPowerLimitReading, isGood)
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_,
                forEachSensorReading(testing::_,
                                     testing::Lt(unsigned(kMaxPlatformNumber)),
                                     testing::_))
            .WillByDefault(testing::Invoke(
                [this](auto type, auto deviceIndex, auto action) {
                    action(*(sensorPowerLimitReading));
                    return true;
                }));

        ON_CALL(
            *powerEfficiencyReading,
            registerReadingConsumerHelper(
                testing::_, ReadingType::platformPowerEfficiency, kAllDevices))
            .WillByDefault(
                testing::SaveArg<0>(&(this->efficiencyReadingConsumer)));

        sut_ = std::make_shared<ReadingAcPlatformLimit>(sensorReadingsManager_,
                                                        powerEfficiencyReading);
        sut_->registerReadingConsumer(readingConsumer,
                                      ReadingType::acPlatformPowerLimit);
    }
};

TEST_F(ReadingAcPowerLimitTest, CorrectReadingProducedExpectValuesInRange)
{
    efficiencyReadingConsumer->updateValue(0.5);
    EXPECT_CALL(*readingConsumer, updateValue(testing::DoubleEq(400)));
    EXPECT_CALL(
        *readingConsumer,
        reportEvent(
            ReadingEventType::readingAvailable,
            testing::AllOf(
                testing::Field(&ReadingContext::type,
                               ReadingType::acPlatformPowerLimit),
                testing::Field(&ReadingContext::deviceIndex, kAllDevices))));
    sut_->run();
}

TEST_F(ReadingAcPowerLimitTest, EfficiencyNanExpectNanReading)
{
    efficiencyReadingConsumer->updateValue(
        std::numeric_limits<double>::quiet_NaN());
    EXPECT_CALL(*readingConsumer,
                updateValue(testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN())));
    sut_->run();
}

TEST_F(ReadingAcPowerLimitTest, LimitNanExpectNanReading)
{
    efficiencyReadingConsumer->updateValue(
        std::numeric_limits<double>::quiet_NaN());
    EXPECT_CALL(*readingConsumer,
                updateValue(testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN())));
    sut_->run();
}

TEST_F(ReadingAcPowerLimitTest, ReadingNotAvailableExpectNanReading)
{
    efficiencyReadingConsumer->updateValue(0.5);
    ON_CALL(*sensorPowerLimitReading, isGood)
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(*readingConsumer,
                updateValue(testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN())));
    sut_->run();
}

TEST_F(ReadingAcPowerLimitTest, EfficiencyNotSetExpectNanReading)
{
    EXPECT_CALL(*readingConsumer,
                updateValue((testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN()))));
    sut_->run();
}