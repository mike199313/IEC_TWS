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
#include "knobs/hwpm_knob.hpp"
#include "mocks/async_executor_mock.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

static const DeviceIndex kDeviceIndex = DeviceIndex{123};
static const uint32_t kDefaultValue = 456;

class HwpmKnobTest : public testing::Test
{
  public:
    virtual ~HwpmKnobTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(
            *asyncExecutorMock_,
            schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                     testing::_, testing::_))
            .WillByDefault(
                testing::DoAll(testing::SaveArg<1>(&asyncTask_),
                               testing::SaveArg<2>(&asyncTaskCallback_)));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(*peciCommands_, setHwpmPreference(testing::_, testing::_))
            .WillByDefault(testing::Return(true));

        sut_ = std::make_shared<HwpmKnob>(
            KnobType::HwpmPerfPreference, kDeviceIndex, kDefaultValue,
            peciCommands_, asyncExecutorMock_, sensorReadingsManager_);
    }

    virtual void TearDown()
    {
    }
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    using HwmonAsyncExecutorMock =
        AsyncExecutorMock<std::pair<KnobType, DeviceIndex>,
                          std::pair<std::ios_base::iostate, uint32_t>>;

    std::shared_ptr<PeciCommandsMock> peciCommands_ =
        std::make_shared<testing::NiceMock<PeciCommandsMock>>();
    std::shared_ptr<HwpmKnob> sut_;
    std::shared_ptr<HwmonAsyncExecutorMock> asyncExecutorMock_ =
        std::make_shared<testing::NiceMock<HwmonAsyncExecutorMock>>();
    HwmonAsyncExecutorMock::Task asyncTask_;
    HwmonAsyncExecutorMock::TaskCallback asyncTaskCallback_;
    testing::NiceMock<testing::MockFunction<bool(uint32_t, uint32_t)>>
        operation_;
};

TEST_F(HwpmKnobTest, task_not_scheduled_when_S0_returns_false)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(0);

    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    sut_->setKnob(5);
    sut_->run();
}

TEST_F(HwpmKnobTest, new_value_saved_with_success)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, 5));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();
    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{5});
    asyncTaskCallback_(ret);
}

TEST_F(HwpmKnobTest, failed_peci_call_will_retrigger_another_set)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(2);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, 5))
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

TEST_F(HwpmKnobTest,
       another_attempt_is_made_when_new_value_was_set_before_callback)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(2);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, 5));
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, 15));

    sut_->setKnob(5);
    sut_->run();

    auto ret = asyncTask_();
    sut_->setKnob(15);
    sut_->run();
    asyncTaskCallback_(ret);
    asyncTaskCallback_(asyncTask_());
}

TEST_F(HwpmKnobTest, clear_knob_works_like_set_knob_but_with_default_value)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, kDefaultValue));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
}

TEST_F(HwpmKnobTest, health_is_ok)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, kDefaultValue));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(HwpmKnobTest, inittialy_health_is_ok)
{
    sut_->run();
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(HwpmKnobTest, health_is_warning)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(kDeviceIndex, kDefaultValue))
        .WillOnce(testing::Return(false));

    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_EQ(sut_->getHealth(), NmHealth::warning);
}

TEST_F(HwpmKnobTest, input_value_out_of_range_throws_logic_exception)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(testing::_, testing::_))
        .Times(0);

    EXPECT_THROW(
        sut_->setKnob(double{std::numeric_limits<uint32_t>::max()} + 1),
        std::logic_error);
}

TEST_F(HwpmKnobTest, incorrect_knob_type_throws_logic_exception)
{
    sut_ = std::make_shared<HwpmKnob>(
        KnobType::TurboRatioLimit, kDeviceIndex, kDefaultValue, peciCommands_,
        asyncExecutorMock_, sensorReadingsManager_);
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::HwpmPerfPreference, kDeviceIndex),
                 testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*peciCommands_, setHwpmPreference(testing::_, testing::_))
        .Times(0);

    EXPECT_THROW(
        sut_->setKnob(double{std::numeric_limits<uint32_t>::max()} + 1),
        std::logic_error);
}

TEST_F(HwpmKnobTest, bias_knob_type_calls_bias_peci_command)
{
    sut_ = std::make_shared<HwpmKnob>(
        KnobType::HwpmPerfBias, kDeviceIndex, kDefaultValue, peciCommands_,
        asyncExecutorMock_, sensorReadingsManager_);
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::HwpmPerfBias, kDeviceIndex),
                         testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SaveArg<1>(&asyncTask_),
                                 testing::SaveArg<2>(&asyncTaskCallback_)));
    EXPECT_CALL(*peciCommands_, setHwpmPreferenceBias(kDeviceIndex, 5))
        .WillOnce(testing::Return(true));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();
    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{5});
    asyncTaskCallback_(ret);
}

TEST_F(HwpmKnobTest, override_knob_type_calls_override_peci_command)
{
    sut_ = std::make_shared<HwpmKnob>(
        KnobType::HwpmPerfPreferenceOverride, kDeviceIndex, kDefaultValue,
        peciCommands_, asyncExecutorMock_, sensorReadingsManager_);
    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::HwpmPerfPreferenceOverride,
                                       kDeviceIndex),
                         testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SaveArg<1>(&asyncTask_),
                                 testing::SaveArg<2>(&asyncTaskCallback_)));
    EXPECT_CALL(*peciCommands_, setHwpmPreferenceOverride(kDeviceIndex, 5))
        .WillOnce(testing::Return(true));

    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();
    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{5});
    asyncTaskCallback_(ret);
}

TEST_F(HwpmKnobTest, set_reserved_fields_in_preference_knob_throws_exception)
{
    sut_ = std::make_shared<HwpmKnob>(
        KnobType::HwpmPerfPreference, kDeviceIndex, kDefaultValue,
        peciCommands_, asyncExecutorMock_, sensorReadingsManager_);
    EXPECT_THROW(sut_->setKnob(kPreferenceReservedFieldsMask),
                 std::logic_error);
}

TEST_F(HwpmKnobTest, set_reserved_fields_in_bias_knob_throws_exception)
{
    sut_ = std::make_shared<HwpmKnob>(
        KnobType::HwpmPerfBias, kDeviceIndex, kDefaultValue, peciCommands_,
        asyncExecutorMock_, sensorReadingsManager_);
    EXPECT_THROW(sut_->setKnob(kBiasReservedFieldsMask), std::logic_error);
}

TEST_F(HwpmKnobTest, set_reserved_fields_in_override_knob_throws_exception)
{
    sut_ = std::make_shared<HwpmKnob>(
        KnobType::HwpmPerfPreferenceOverride, kDeviceIndex, kDefaultValue,
        peciCommands_, asyncExecutorMock_, sensorReadingsManager_);
    EXPECT_THROW(sut_->setKnob(kOverrideReservedFieldsMask), std::logic_error);
}

TEST_F(HwpmKnobTest, InitiallyIsKnobSetReturnsFalse)
{
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(HwpmKnobTest, SetKnobIsKnobSetReturnsTrue)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_TRUE(sut_->isKnobSet());
}

TEST_F(HwpmKnobTest, SetKnobFailedIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->setKnob(6);
    sut_->run();
    asyncTaskCallback_({std::ios_base::badbit, 0.0});
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(HwpmKnobTest, ResetKnobIsKnobSetReturnsFalse)
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