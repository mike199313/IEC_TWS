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
#include "knobs/prochot_ratio_knob.hpp"
#include "mocks/async_executor_mock.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "utils/common_test_settings.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

class ProchotRatioKnobTest : public testing::Test
{
  public:
    virtual ~ProchotRatioKnobTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
            .WillByDefault(
                testing::DoAll(testing::SaveArg<1>(&asyncTask_),
                               testing::SaveArg<2>(&asyncTaskCallback_)));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(*peciCommands_, setProchotRatio(testing::_, testing::_))
            .WillByDefault(testing::Return(true));

        ON_CALL(*peciCommands_, getCpuId(DeviceIndex{0}))
            .WillByDefault(testing::Return(kCpuIdDefault));

        sut_ = std::make_shared<ProchotRatioKnob>(
            KnobType::Prochot, DeviceIndex{0}, peciCommands_,
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

    std::shared_ptr<ProchotRatioKnob> sut_;
    std::shared_ptr<PeciCommandsMock> peciCommands_ =
        std::make_shared<testing::NiceMock<PeciCommandsMock>>();
    std::map<SensorReadingType, std::vector<std::shared_ptr<SensorReadingMock>>>
        sensorReadings_;
    std::shared_ptr<HwmonAsyncExecutorMock> asyncExecutorMock_ =
        std::make_shared<testing::NiceMock<HwmonAsyncExecutorMock>>();
    HwmonAsyncExecutorMock::Task asyncTask_;
    HwmonAsyncExecutorMock::TaskCallback asyncTaskCallback_;
};

TEST_F(ProchotRatioKnobTest, NewValueSavedWithSuccess)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setProchotRatio(0, 5));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();

    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{5});
    asyncTaskCallback_(ret);
}

TEST_F(ProchotRatioKnobTest, TaskNotScheduledWhenS0ReturnsFalse)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(0);

    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    sut_->setKnob(5);
    sut_->run();
}

TEST_F(ProchotRatioKnobTest, TaskNotScheduledWhenCpuIsUnavailable)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(0);

    ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
        .WillByDefault(testing::Return(false));

    sut_->setKnob(5);
    sut_->run();
}

TEST_F(ProchotRatioKnobTest, FailedPeciCallWillRetriggerAnotherSet)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(2);
    EXPECT_CALL(*peciCommands_, setProchotRatio(0, 5))
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

TEST_F(ProchotRatioKnobTest, InputValueOutOfRangeThrowsLogicException)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*peciCommands_, setProchotRatio(0, 5)).Times(0);

    EXPECT_THROW(sut_->setKnob(double{std::numeric_limits<uint8_t>::max() + 1}),
                 std::logic_error);
}

TEST_F(ProchotRatioKnobTest,
       AnotherAttemptIsMadeWhenNewValueWasSetBeforeCallback)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(2);

    EXPECT_CALL(*peciCommands_, setProchotRatio(0, 5));
    EXPECT_CALL(*peciCommands_, setProchotRatio(0, 15));

    sut_->setKnob(5);
    sut_->run();

    auto ret = asyncTask_();
    sut_->setKnob(15);
    sut_->run();
    asyncTaskCallback_(ret);
    asyncTaskCallback_(asyncTask_());
}

TEST_F(ProchotRatioKnobTest, ClearKnobCallsPeciWithDefaultValueTakenFromCpu)
{
    uint8_t defaultValue = 5;
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(2);

    EXPECT_CALL(*peciCommands_, getMaxEfficiencyRatio(0, kCpuIdDefault))
        .WillOnce(testing::Return(std::optional<uint8_t>(defaultValue)));

    EXPECT_CALL(*peciCommands_, setProchotRatio(0, defaultValue));

    sut_->resetKnob();
    asyncTaskCallback_(asyncTask_());
    sut_->run();
    asyncTaskCallback_(asyncTask_());
}

TEST_F(ProchotRatioKnobTest, ClearKnobCallsPeciWithHardcodedDefaultValue)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(2);

    EXPECT_CALL(*peciCommands_, getMaxEfficiencyRatio(0, kCpuIdDefault))
        .WillOnce(testing::Return(std::nullopt));

    EXPECT_CALL(*peciCommands_, setProchotRatio(0, kDefaultProchotValue));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->run();
    asyncTaskCallback_(asyncTask_());
}

TEST_F(ProchotRatioKnobTest, HealthIsOkWhenCannotGetDefault)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(2);
    EXPECT_CALL(*peciCommands_, getMaxEfficiencyRatio(0, kCpuIdDefault))
        .WillOnce(testing::Return(std::nullopt));

    EXPECT_CALL(*peciCommands_, setProchotRatio(0, kDefaultProchotValue));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(ProchotRatioKnobTest, InittialyHealthIsOk)
{
    sut_->run();
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(ProchotRatioKnobTest, HealthIsWarning)
{
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::Prochot, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(2);
    EXPECT_CALL(*peciCommands_, setProchotRatio(0, kDefaultProchotValue))
        .WillOnce(testing::Return(false));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_EQ(sut_->getHealth(), NmHealth::warning);
}

TEST_F(ProchotRatioKnobTest, InitiallyIsKnobSetReturnsFalse)
{
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(ProchotRatioKnobTest, SetKnobIsKnobSetReturnsTrue)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_TRUE(sut_->isKnobSet());
}

TEST_F(ProchotRatioKnobTest, SetKnobFailedIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->setKnob(6);
    sut_->run();
    asyncTaskCallback_({std::ios_base::badbit, 0.0});
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(ProchotRatioKnobTest, ResetKnobIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());

    ON_CALL(*peciCommands_, getMaxEfficiencyRatio(0, kCpuIdDefault))
        .WillByDefault(testing::Return(std::optional<uint8_t>(3)));
    sut_->resetKnob();
    asyncTaskCallback_(asyncTask_());
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_FALSE(sut_->isKnobSet());
}

} // namespace nodemanager