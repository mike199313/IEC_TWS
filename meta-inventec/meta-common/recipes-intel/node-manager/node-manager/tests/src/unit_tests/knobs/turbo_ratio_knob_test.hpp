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
#include "common_types.hpp"
#include "knobs/turbo_ratio_knob.hpp"
#include "mocks/async_executor_mock.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

class TurboRatioKnobTest : public testing::Test
{
  public:
    virtual ~TurboRatioKnobTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(
            *asyncExecutorMock_,
            schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                     testing::_, testing::_))
            .WillByDefault(
                testing::DoAll(testing::SaveArg<1>(&asyncTask_),
                               testing::SaveArg<2>(&asyncTaskCallback_)));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(*peciCommands_, setTurboRatio(testing::_, testing::_))
            .WillByDefault(testing::Return(true));

        sut_ = std::make_shared<TurboRatioKnob>(
            KnobType::TurboRatioLimit, DeviceIndex{0}, peciCommands_,
            asyncExecutorMock_, sensorReadingsManager_);
    }

    virtual void TearDown()
    {
    }
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    using HwmonAsyncExecutorMock =
        AsyncExecutorMock<std::pair<KnobType, DeviceIndex>,
                          std::pair<std::ios_base::iostate, uint32_t>>;

    std::shared_ptr<TurboRatioKnob> sut_;
    std::shared_ptr<PeciCommandsMock> peciCommands_ =
        std::make_shared<testing::NiceMock<PeciCommandsMock>>();
    std::shared_ptr<HwmonAsyncExecutorMock> asyncExecutorMock_ =
        std::make_shared<testing::NiceMock<HwmonAsyncExecutorMock>>();
    HwmonAsyncExecutorMock::Task asyncTask_;
    HwmonAsyncExecutorMock::TaskCallback asyncTaskCallback_;
};

TEST_F(TurboRatioKnobTest, NewValueSavedWithSuccess)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 5));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();

    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{5});
    asyncTaskCallback_(ret);
}

TEST_F(TurboRatioKnobTest, FailedPeciCallWillRetriggerAnotherSet)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(2);
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 5))
        .WillOnce(testing::Return(false));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();
    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::failbit);
    EXPECT_EQ(savedValue, uint32_t{0});
    asyncTaskCallback_(ret);
    sut_->run();
}

TEST_F(TurboRatioKnobTest, TaskNotScheduledWhenS0ReturnsFalse)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(0);

    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    sut_->setKnob(5);
    sut_->run();
}

TEST_F(TurboRatioKnobTest, TaskNotScheduledWhenCpuIsUnavailable)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(0);

    ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
        .WillByDefault(testing::Return(false));

    sut_->setKnob(5);
    sut_->run();
}

TEST_F(TurboRatioKnobTest, InputValueOutOfRangeThrowsLogicException)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 5)).Times(0);

    EXPECT_THROW(sut_->setKnob(double{std::numeric_limits<uint8_t>::max() + 1}),
                 std::logic_error);
}

TEST_F(TurboRatioKnobTest, AnotherAttemptIsMadeWhenNewValueWasSetBeforeCallback)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(2);

    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 5));
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 15));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();
    sut_->setKnob(15);
    sut_->run();
    asyncTaskCallback_(ret);
    asyncTaskCallback_(asyncTask_());
}

TEST_F(TurboRatioKnobTest, ClearKnobWorksLikeSetKnobButWithFF)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 0xFF));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
}

TEST_F(TurboRatioKnobTest, HealthIsOK)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 0xFF));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(TurboRatioKnobTest, InittialyHealthIsOk)
{
    sut_->run();
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(TurboRatioKnobTest, HealthIsWarning)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::TurboRatioLimit, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setTurboRatio(0, 0xFF))
        .WillOnce(testing::Return(false));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_EQ(sut_->getHealth(), NmHealth::warning);
}

TEST_F(TurboRatioKnobTest, InitiallyIsKnobSetReturnsFalse)
{
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(TurboRatioKnobTest, SetKnobIsKnobSetReturnsTrue)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_TRUE(sut_->isKnobSet());
}

TEST_F(TurboRatioKnobTest, SetKnobFailedIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->setKnob(6);
    sut_->run();
    asyncTaskCallback_({std::ios_base::badbit, 0.0});
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(TurboRatioKnobTest, ResetKnobIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_FALSE(sut_->isKnobSet());
}

} // namespace nodemanager