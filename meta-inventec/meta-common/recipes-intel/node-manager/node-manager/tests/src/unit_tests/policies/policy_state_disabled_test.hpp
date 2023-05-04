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
class PolicyDisabledStateTest : public ::testing::Test
{
  protected:
    PolicyDisabledStateTest() = default;
    virtual ~PolicyDisabledStateTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(testing::Const(*policy_), getShortObjectPath())
            .WillByDefault(testing::ReturnRef(shortObjectPath_));
        sut_->initialize(policy_, DbusEnvironment::getBus());
    }

    virtual void TearDown() override
    {
        ASSERT_TRUE(DbusEnvironment::waitForAllFutures());
        sut_ = nullptr;
        ASSERT_TRUE(DbusEnvironment::waitForAllFutures());
    }

    std::shared_ptr<PolicyMock> policy_ =
        std::make_shared<testing::NiceMock<PolicyMock>>();
    std::unique_ptr<PolicyStateIf> sut_ =
        std::make_unique<PolicyStateDisabled>();
    std::string shortObjectPath_ = "/Domain/0/Policy/0";
};

TEST_F(PolicyDisabledStateTest, OnInit_uninstallTriggers)
{
    sut_ = std::make_unique<PolicyStateDisabled>();
    EXPECT_CALL(*policy_, uninstallTrigger());
    sut_->initialize(policy_, DbusEnvironment::getBus());
}

TEST_F(PolicyDisabledStateTest,
       OnDestruction_postOnStateChanged_And_ValidateParameters)
{
    EXPECT_CALL(*policy_, onStateChanged())
        .WillOnce(testing::DoAll(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("onStateChanged_called"))));
    EXPECT_CALL(*policy_, validateParameters())
        .WillOnce(testing::DoAll(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("validateParameters_called"))));

    sut_ = nullptr;
    DbusEnvironment::waitForFuture("onStateChanged_called");
    DbusEnvironment::waitForFuture("validateParameters_called");
}

TEST_F(PolicyDisabledStateTest, OnLimitSelection_False)
{
    auto newState = sut_->onLimitSelection(false);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, OnLimitSelection_True)
{
    auto newState = sut_->onLimitSelection(true);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, OnTriggerAction_activated)
{
    auto newState = sut_->onTriggerAction(TriggerActionType::trigger);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, OnTriggerAction_deactivate)
{
    auto newState = sut_->onTriggerAction(TriggerActionType::deactivate);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, OnTriggerAction_missingReading)
{
    auto newState = sut_->onTriggerAction(TriggerActionType::missingReading);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, OnParametersValidation_False)
{
    auto newState = sut_->onParametersValidation(false);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, OnParametersValidation_True)
{
    auto newState = sut_->onParametersValidation(true);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, onEnabled_False)
{
    auto newState = sut_->onEnabled(false);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, onEnabled_True)
{
    auto newState = sut_->onEnabled(true);
    EXPECT_TRUE(newState);
    EXPECT_EQ(newState->getState(), PolicyState::pending);
}

TEST_F(PolicyDisabledStateTest, onParentEnabled_False)
{
    auto newState = sut_->onParentEnabled(false);
    EXPECT_FALSE(newState);
}

TEST_F(PolicyDisabledStateTest, onParentEnabled_True)
{
    auto newState = sut_->onParentEnabled(true);
    EXPECT_FALSE(newState);
}
