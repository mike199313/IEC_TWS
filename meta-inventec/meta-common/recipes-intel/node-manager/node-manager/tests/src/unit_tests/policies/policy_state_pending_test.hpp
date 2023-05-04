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

#include "mocks/policy_mock.hpp"
#include "policies/policy_state.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class PolicyPendingStateTest : public ::testing::Test
{
  protected:
    PolicyPendingStateTest() = default;
    virtual ~PolicyPendingStateTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(testing::Const(*policy_), getShortObjectPath())
            .WillByDefault(testing::ReturnRef(shortObjectPath_));
        sut_->initialize(policy_, DbusEnvironment::getBus());
    }
    std::shared_ptr<PolicyMock> policy_ =
        std::make_shared<testing::NiceMock<PolicyMock>>();
    std::unique_ptr<PolicyStateIf> sut_ =
        std::make_unique<PolicyStatePending>();
    std::string shortObjectPath_ = "/Domain/0/Policy/0";
};

TEST_F(PolicyPendingStateTest, OnInit_uninstallTriggers)
{
    sut_ = std::make_unique<PolicyStatePending>();
    EXPECT_CALL(*policy_, uninstallTrigger());
    sut_->initialize(policy_, DbusEnvironment::getBus());
}

TEST_F(PolicyPendingStateTest, OnLimitSelection_False)
{
    auto newState = sut_->onLimitSelection(false);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, OnLimitSelection_True)
{
    auto newState = sut_->onLimitSelection(true);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, OnTriggerAction_activated)
{
    auto newState = sut_->onTriggerAction(TriggerActionType::trigger);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, OnTriggerAction_deactivate)
{
    auto newState = sut_->onTriggerAction(TriggerActionType::deactivate);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, OnTriggerAction_missingReading)
{
    auto newState = sut_->onTriggerAction(TriggerActionType::missingReading);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, OnParametersValidation_False)
{
    auto newState = sut_->onParametersValidation(false);
    EXPECT_TRUE(newState);
    EXPECT_EQ(newState->getState(), PolicyState::suspended);
}

TEST_F(PolicyPendingStateTest, OnParametersValidation_True)
{
    auto newState = sut_->onParametersValidation(true);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, onEnabled_False)
{
    auto newState = sut_->onEnabled(false);
    EXPECT_TRUE(newState);
    EXPECT_EQ(newState->getState(), PolicyState::disabled);
}

TEST_F(PolicyPendingStateTest, onEnabled_True)
{
    auto newState = sut_->onEnabled(true);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, onParentEnabled_False)
{
    auto newState = sut_->onParentEnabled(false);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyPendingStateTest, onParentEnabled_True)
{
    auto newState = sut_->onParentEnabled(true);
    EXPECT_TRUE(newState);
    EXPECT_EQ(newState->getState(), PolicyState::ready);
}
