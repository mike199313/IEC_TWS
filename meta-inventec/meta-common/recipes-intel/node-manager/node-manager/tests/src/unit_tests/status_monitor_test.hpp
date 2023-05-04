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
#include "mocks/devices_manager_mock.hpp"
#include "mocks/status_monitor_action_mock.hpp"
#include "status_monitor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class StatusMonitorSensorMissingTest : public ::testing::Test
{
  protected:
    StatusMonitorSensorMissingTest()
    {
    }

    virtual void SetUp() override
    {
        EXPECT_CALL(*actions_, logSensorReadingMissing(testing::_, testing::_))
            .Times(0);
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::acPlatformPower, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&readingAcPlatformPower_));
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::volumetricAirflow, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&volumetricAirflow_));

        sut_ = std::make_unique<StatusMonitor>(devManMock_, actions_);
    }

    virtual ~StatusMonitorSensorMissingTest() = default;

    DeviceIndex deviceIndex_ = 0;
    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> readingAcPlatformPower_;
    std::shared_ptr<ReadingConsumer> volumetricAirflow_;
    std::shared_ptr<StatusMonitorActionMock> actions_ =
        std::make_shared<testing::NiceMock<StatusMonitorActionMock>>();
    std::unique_ptr<StatusMonitor> sut_;
};

TEST_F(StatusMonitorSensorMissingTest,
       StatusMonitorDestructionUnregisteresAllReadingConsumerInDevicesManager)
{
    EXPECT_CALL(*devManMock_, unregisterReadingConsumer(testing::_)).Times(11);
    sut_ = nullptr;
}

TEST_F(StatusMonitorSensorMissingTest, NoActionBeforeTimeWindowEnds)
{
    EXPECT_CALL(*actions_, logSensorMissing(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*actions_, logReadingMissing(testing::_)).Times(0);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, kAllDevices});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, kAllDevices});

    Clock::stepSec(19);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest, NoActionWhenNeverAppeared)
{
    EXPECT_CALL(*actions_, logSensorMissing(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*actions_, logReadingMissing(testing::_)).Times(0);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, kAllDevices});

    Clock::stepSec(19);
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, kAllDevices});

    Clock::stepSec(2);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest,
       ActionIsTriggeredOnlyWhen20secTimeWindowEnds)
{
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});

    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_))
        .Times(0);
    Clock::stepSec(19);
    sut_->run();

    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_))
        .Times(1);
    Clock::stepSec(2);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest,
       FollowingSensorDisappearEventsDoNotResetTimeWindow)
{
    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_))
        .Times(1);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(18);
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(3);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest,
       OnlyOneActionIsCalledOnMultipleEventSensorDisappear)
{
    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_))
        .Times(1);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(21);
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest,
       SecondActionIsTriggeredOnlyOnWhenOpositeEventOccurs)
{
    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_));
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(21);
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();

    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_));
    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest, NoActionOnSensorAppear)
{
    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_))
        .Times(0);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorSensorMissingTest, TwoActionsOnTwoDifferentReadingTypes)
{
    EXPECT_CALL(*actions_,
                logSensorMissing(enumToStr(kSensorReadingTypeNames,
                                           SensorReadingType::acPlatformPower),
                                 deviceIndex_));
    EXPECT_CALL(*actions_, logSensorMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::volumetricAirflow),
                               2));

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorAppear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    volumetricAirflow_->reportEvent(SensorEventType::sensorAppear,
                                    {SensorReadingType::volumetricAirflow, 2},
                                    {ReadingType::volumetricAirflow, 2});

    readingAcPlatformPower_->reportEvent(
        SensorEventType::sensorDisappear,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    volumetricAirflow_->reportEvent(SensorEventType::sensorDisappear,
                                    {SensorReadingType::volumetricAirflow, 2},
                                    {ReadingType::volumetricAirflow, 2});
    Clock::stepSec(21);
    sut_->run();
}

class StatusMonitorSensorReadingMissingTest
    : public StatusMonitorSensorMissingTest
{
  protected:
    StatusMonitorSensorReadingMissingTest() = default;

    virtual void SetUp() override
    {
        EXPECT_CALL(*actions_, logSensorMissing(testing::_, testing::_))
            .Times(0);
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::acPlatformPower, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&readingAcPlatformPower_));
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::volumetricAirflow, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&volumetricAirflow_));

        sut_ = std::make_unique<StatusMonitor>(devManMock_, actions_);
    }

    virtual ~StatusMonitorSensorReadingMissingTest() = default;
};

TEST_F(StatusMonitorSensorReadingMissingTest, NoActionWhenNeverAppeared)
{
    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::acPlatformPower),
                               deviceIndex_))
        .Times(0);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
}

TEST_F(StatusMonitorSensorReadingMissingTest, ActionIsTriggeredImmediatelly)
{
    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::acPlatformPower),
                               deviceIndex_))
        .Times(1);
    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingAvailable,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
}

TEST_F(StatusMonitorSensorReadingMissingTest, OnlySingleActionOnMultipleEvents)
{
    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::acPlatformPower),
                               deviceIndex_))
        .Times(1);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingAvailable,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
}

TEST_F(StatusMonitorSensorReadingMissingTest,
       SecondActionIsTriggeredOnlyOnWhenOpositeEventOccurs)
{
    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingAvailable,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});

    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::acPlatformPower),
                               deviceIndex_));

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingAvailable,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();

    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::acPlatformPower),
                               deviceIndex_));

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
}

TEST_F(StatusMonitorSensorReadingMissingTest, NoActionOnReadingAvailable)
{
    EXPECT_CALL(*actions_, logSensorReadingMissing(testing::_, testing::_))
        .Times(0);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingAvailable,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    sut_->run();
}

TEST_F(StatusMonitorSensorReadingMissingTest,
       TwoActionsOnTwoDifferentReadingTypes)
{
    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::acPlatformPower),
                               deviceIndex_))
        .Times(1);
    EXPECT_CALL(*actions_, logSensorReadingMissing(
                               enumToStr(kSensorReadingTypeNames,
                                         SensorReadingType::volumetricAirflow),
                               2))
        .Times(1);

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingAvailable,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    volumetricAirflow_->reportEvent(SensorEventType::readingAvailable,
                                    {SensorReadingType::volumetricAirflow, 2},
                                    {ReadingType::volumetricAirflow, 2});

    readingAcPlatformPower_->reportEvent(
        SensorEventType::readingMissing,
        {SensorReadingType::acPlatformPower, deviceIndex_},
        {ReadingType::acPlatformPower, deviceIndex_});
    volumetricAirflow_->reportEvent(SensorEventType::readingMissing,
                                    {SensorReadingType::volumetricAirflow, 2},
                                    {ReadingType::volumetricAirflow, 2});
    sut_->run();
}

class StatusMonitorReadingMissingTest : public ::testing::Test
{
  protected:
    StatusMonitorReadingMissingTest()
    {
    }

    virtual void SetUp() override
    {
        EXPECT_CALL(*actions_, logReadingMissing(testing::_)).Times(0);
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::acPlatformPower, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&readingAcPlatformPower_));
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::volumetricAirflow, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&volumetricAirflow_));
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::pciePower, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&pciePower_));

        sut_ = std::make_unique<StatusMonitor>(devManMock_, actions_);
    }

    virtual ~StatusMonitorReadingMissingTest() = default;

    DeviceIndex deviceIndex_ = kAllDevices;
    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> readingAcPlatformPower_;
    std::shared_ptr<ReadingConsumer> volumetricAirflow_;
    std::shared_ptr<ReadingConsumer> pciePower_;
    std::shared_ptr<StatusMonitorActionMock> actions_ =
        std::make_shared<testing::NiceMock<StatusMonitorActionMock>>();
    std::unique_ptr<StatusMonitor> sut_;
};

TEST_F(StatusMonitorReadingMissingTest,
       StatusMonitorDestructionUnregisteresAllReadingConsumerInDevicesManager)
{
    EXPECT_CALL(*devManMock_, unregisterReadingConsumer(testing::_)).Times(11);
    sut_ = nullptr;
}

TEST_F(StatusMonitorReadingMissingTest, NoActionBeforeTimeWindowEnds)
{
    EXPECT_CALL(*actions_, logReadingMissing(testing::_)).Times(0);

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(19);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest, NoActionWhenNeverAppeared)
{
    EXPECT_CALL(*actions_, logReadingMissing(testing::_)).Times(0);

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, kAllDevices});

    Clock::stepSec(19);
    sut_->run();

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, kAllDevices});

    Clock::stepSec(2);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest,
       ActionIsTriggeredOnlyWhen20secTimeWindowEnds)
{
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    EXPECT_CALL(*actions_,
                logReadingMissing(
                    enumToStr(kReadingTypeNames, ReadingType::acPlatformPower)))
        .Times(0);
    Clock::stepSec(19);
    sut_->run();

    EXPECT_CALL(*actions_,
                logReadingMissing(
                    enumToStr(kReadingTypeNames, ReadingType::acPlatformPower)))
        .Times(1);
    Clock::stepSec(2);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest,
       FollowingReadingDisappearEventsDoNotResetTimeWindow)
{
    EXPECT_CALL(*actions_,
                logReadingMissing(
                    enumToStr(kReadingTypeNames, ReadingType::acPlatformPower)))
        .Times(1);

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(18);
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(3);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest,
       OnlyOneActionIsCalledOnMultipleEventReadingDisappear)
{
    EXPECT_CALL(*actions_,
                logReadingMissing(
                    enumToStr(kReadingTypeNames, ReadingType::acPlatformPower)))
        .Times(1);

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(21);
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest,
       SecondActionIsTriggeredOnlyOnWhenOpositeEventOccurs)
{
    EXPECT_CALL(*actions_,
                logReadingMissing(enumToStr(kReadingTypeNames,
                                            ReadingType::acPlatformPower)));
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    Clock::stepSec(21);
    sut_->run();
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    sut_->run();

    EXPECT_CALL(*actions_,
                logReadingMissing(enumToStr(kReadingTypeNames,
                                            ReadingType::acPlatformPower)));
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest, NoActionOnReadingAppear)
{
    EXPECT_CALL(*actions_,
                logReadingMissing(
                    enumToStr(kReadingTypeNames, ReadingType::acPlatformPower)))
        .Times(0);

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});

    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest, TwoActionsOnTwoDifferentReadingTypes)
{
    EXPECT_CALL(*actions_,
                logReadingMissing(enumToStr(kReadingTypeNames,
                                            ReadingType::acPlatformPower)));
    EXPECT_CALL(*actions_,
                logNonCriticalReadingMissing(enumToStr(
                    kReadingTypeNames, ReadingType::volumetricAirflow)));

    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::acPlatformPower, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingAvailable,
        {ReadingType::volumetricAirflow, deviceIndex_});
    readingAcPlatformPower_->reportEvent(
        ReadingEventType::readingUnavailable,
        {ReadingType::volumetricAirflow, deviceIndex_});

    Clock::stepSec(21);
    sut_->run();
}

TEST_F(StatusMonitorReadingMissingTest, InvalidDeviceIdExpectLogicError)
{
    EXPECT_THROW(
        readingAcPlatformPower_->reportEvent(ReadingEventType::readingAvailable,
                                             {ReadingType::acPlatformPower, 9}),
        std::logic_error);
}

TEST_F(StatusMonitorReadingMissingTest, ActionOnPcieExpectNonCriticalLog)
{
    EXPECT_CALL(*actions_, logNonCriticalReadingMissing(enumToStr(
                               kReadingTypeNames, ReadingType::pciePower)))
        .Times(1);

    pciePower_->reportEvent(ReadingEventType::readingAvailable,
                            {ReadingType::pciePower, deviceIndex_});
    pciePower_->reportEvent(ReadingEventType::readingUnavailable,
                            {ReadingType::pciePower, deviceIndex_});

    Clock::stepSec(21);
    sut_->run();
}