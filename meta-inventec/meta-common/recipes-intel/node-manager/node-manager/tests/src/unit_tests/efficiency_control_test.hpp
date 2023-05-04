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

#include "efficiency_control.hpp"
#include "knobs/knob.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class EfficiencyControlTest : public ::testing::Test
{
  public:
    EfficiencyControlTest()
    {
    }

  protected:
    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();

    std::unique_ptr<EfficiencyControl> sut_ =
        std::make_unique<EfficiencyControl>(devManMock_);
};

TEST_F(EfficiencyControlTest, CreateEfficiencyControlExpectNoThrow)
{
    EXPECT_NO_THROW(std::make_unique<EfficiencyControl>(devManMock_));
}

TEST_F(EfficiencyControlTest, SetValueTurboRatioLimitExpectSetKnobValue)
{
    EXPECT_CALL(*devManMock_,
                setKnobValue(KnobType::TurboRatioLimit, kAllDevices, 10))
        .Times(1);

    sut_->setValue(KnobType::TurboRatioLimit, 10);
}

TEST_F(EfficiencyControlTest, SetValueHwpmPerfPreferenceExpectSetKnobValue)
{
    EXPECT_CALL(*devManMock_,
                setKnobValue(KnobType::HwpmPerfPreference, kAllDevices, 10))
        .Times(1);

    sut_->setValue(KnobType::HwpmPerfPreference, 10);
}

TEST_F(EfficiencyControlTest, ResetValueTurboRatioLimitExpectResetKnobValue)
{
    EXPECT_CALL(*devManMock_,
                resetKnobValue(KnobType::TurboRatioLimit, kAllDevices))
        .Times(1);

    sut_->resetValue(KnobType::TurboRatioLimit);
}