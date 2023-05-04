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
#include "mocks/limit_exception_handler_mock.hpp"
#include "mocks/policy_mock.hpp"
#include "mocks/statistic_mock.hpp"
#include "policies/limit_exception_monitor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class LimitExceptionMonitorTest : public ::testing::Test
{
  protected:
    LimitExceptionMonitorTest()
    {
    }

    virtual void SetUp() override
    {
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPackagePower, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&reading_));
        ON_CALL(*policy_, getState)
            .WillByDefault(testing::Return(PolicyState::selected));

        limitProp_.set(limit_);
        correctionTimeProp_.set(correctionTimeMs_);
        limitExceptionProp_.set(limitException_);

        sut_ = std::make_unique<LimitExceptionMonitor>(
            policy_, limitExceptionHanderMock_, devManMock_,
            ReadingType::cpuPackagePower, limitProp_, correctionTimeProp_,
            limitExceptionProp_);
    }

    virtual ~LimitExceptionMonitorTest() = default;
    std::shared_ptr<PolicyMock> policy_ =
        std::make_shared<testing::NiceMock<PolicyMock>>();
    std::shared_ptr<LimitExceptionHandlerMock> limitExceptionHanderMock_ =
        std::make_shared<testing::NiceMock<LimitExceptionHandlerMock>>();
    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> reading_;
    std::unique_ptr<LimitExceptionMonitor> sut_;
    uint16_t limit_ = 10;
    double limitOffset_ = std::max(1.05 * limit_, 2.0);
    double aboveLimitOffset_ = limitOffset_ + 1.0;
    double belowLimitOffset_ = limit_;
    uint32_t correctionTimeMs_ = 5000;
    LimitException limitException_ = LimitException::powerOff;
    PropertyWrapper<uint16_t> limitProp_;
    PropertyWrapper<uint32_t> correctionTimeProp_;
    PropertyWrapper<LimitException> limitExceptionProp_;
};

TEST_F(LimitExceptionMonitorTest, UnregisterInDescrutcor)
{
    EXPECT_CALL(*devManMock_,
                unregisterReadingConsumer(testing::Truly(
                    [this](const auto& arg) { return arg == reading_; })));
    sut_ = nullptr;
}

TEST_F(LimitExceptionMonitorTest, AllTimeBelowLimitThenNoPowerOffAction)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    reading_->updateValue(belowLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    reading_->updateValue(belowLimitOffset_);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest, AllTimeAboveLimitThenPowerOffAction)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_));

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       AllTimeAboveLimitOnTwoXCorrectionTimeNoResetAndPowerOffActionOnce)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(1);

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       AllTimeAboveLimitOnTwoXCorrectionTimeResetAndPowerOffActionTwice)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(2);

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();

    sut_->reset();
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       AboveLimitAndBackBelowWithinTimeLimitThenNoPowerOffAction)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(3);
    reading_->updateValue(belowLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       AboveLimitAndBackBelowAndAboveAgainThenPowerOffAction)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_));

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(1);
    reading_->updateValue(belowLimitOffset_);
    sut_->run();

    Clock::stepSec(1);
    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       BetweenLimitAndLimitOffsetThenNoPowerOffAction)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    reading_->updateValue(limitOffset_ - 0.1);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest, BelowMinLimitOffsetThenNoPowerOffAction)
{
    std::shared_ptr<ReadingConsumer> rc;
    ON_CALL(*devManMock_,
            registerReadingConsumerHelper(
                testing::_, ReadingType::cpuPackagePower, kAllDevices))
        .WillByDefault(testing::SaveArg<0>(&rc));

    limitProp_.set(limit_ = uint16_t{1});
    std::unique_ptr<LimitExceptionMonitor> limitSelector =
        std::make_unique<LimitExceptionMonitor>(
            policy_, limitExceptionHanderMock_, devManMock_,
            ReadingType::cpuPackagePower, limitProp_, correctionTimeProp_,
            limitExceptionProp_);

    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    rc->updateValue(1.9);
    limitSelector->run();

    Clock::stepSec(6);
    limitSelector->run();
}

TEST_F(LimitExceptionMonitorTest, AboveMinLimitOffsetThenPowerOffAction)
{
    std::shared_ptr<ReadingConsumer> rc;
    ON_CALL(*devManMock_,
            registerReadingConsumerHelper(
                testing::_, ReadingType::cpuPackagePower, kAllDevices))
        .WillByDefault(testing::SaveArg<0>(&rc));

    limitProp_.set(limit_ = uint16_t{1});
    std::unique_ptr<LimitExceptionMonitor> limitSelector =
        std::make_unique<LimitExceptionMonitor>(
            policy_, limitExceptionHanderMock_, devManMock_,
            ReadingType::cpuPackagePower, limitProp_, correctionTimeProp_,
            limitExceptionProp_);

    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_));

    rc->updateValue(2.1);
    limitSelector->run();

    Clock::stepSec(6);
    limitSelector->run();
}

TEST_F(LimitExceptionMonitorTest,
       NoExceptionHandlerThenNoPowerOffActionDespiteException)
{
    std::shared_ptr<ReadingConsumer> rc;
    ON_CALL(*devManMock_,
            registerReadingConsumerHelper(
                testing::_, ReadingType::cpuPackagePower, kAllDevices))
        .WillByDefault(testing::SaveArg<0>(&rc));

    std::unique_ptr<LimitExceptionMonitor> limitSelector =
        std::make_unique<LimitExceptionMonitor>(
            policy_, nullptr, devManMock_, ReadingType::cpuPackagePower,
            limitProp_, correctionTimeProp_, limitExceptionProp_);

    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    rc->updateValue(aboveLimitOffset_);
    limitSelector->run();

    Clock::stepSec(6);
    limitSelector->run();
}

TEST_F(LimitExceptionMonitorTest, NanValue)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    reading_->updateValue(std::numeric_limits<double>::quiet_NaN());
    ASSERT_NO_THROW(sut_->run());

    Clock::stepSec(6);
    reading_->updateValue(std::numeric_limits<double>::quiet_NaN());
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(LimitExceptionMonitorTest, OnlyOneActionCall)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .WillOnce(
            testing::InvokeArgument<1>(boost::system::errc::errc_t::success));

    reading_->updateValue(aboveLimitOffset_);
    ASSERT_NO_THROW(sut_->run());

    Clock::stepSec(6);
    ASSERT_NO_THROW(sut_->run());

    Clock::stepSec(6);
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(LimitExceptionMonitorTest,
       NoPowerOffActionWhenLimitExceededAndPolicyStateNotSelected)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(0);

    for (auto policyState : kPolicyStateSet)
    {
        if (policyState != PolicyState::selected)
        {
            ON_CALL(*policy_, getState)
                .WillByDefault(testing::Return(PolicyState::selected));
            reading_->updateValue(aboveLimitOffset_);
            sut_->run();

            ON_CALL(*policy_, getState)
                .WillByDefault(testing::Return(policyState));
            Clock::stepSec(6);
            sut_->run();
        }
    }
}

TEST_F(LimitExceptionMonitorTest,
       PowerOffActionWhenLimitExceededAndPolicyStateSelected)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_));

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    ON_CALL(*policy_, getState)
        .WillByDefault(testing::Return(PolicyState::selected));
    Clock::stepSec(3);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest, ProlongedHandlerAction)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_));

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       PowerOffActionCalledTwiceWhenLimitIsCrossedTwice)
{
    LimitExceptionActionCallback handerCallback_;
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_))
        .Times(2)
        .WillRepeatedly(testing::SaveArg<1>(&handerCallback_));

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
    handerCallback_(boost::system::errc::errc_t::success);

    Clock::stepSec(1);
    reading_->updateValue(belowLimitOffset_);
    sut_->run(); // this call will reset the monitor

    Clock::stepSec(1);
    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run();
}

TEST_F(LimitExceptionMonitorTest,
       PowerOffActionCalledOnceWhenPreviousActionNotFinisedAndLimitCrossedTwice)
{
    EXPECT_CALL(*limitExceptionHanderMock_,
                doAction(LimitException::powerOff, testing::_));

    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run(); // this call will trigger action callback

    Clock::stepSec(1);
    reading_->updateValue(belowLimitOffset_);
    sut_->run(); // this call will reset the monitor

    Clock::stepSec(1);
    reading_->updateValue(aboveLimitOffset_);
    sut_->run();

    Clock::stepSec(6);
    sut_->run(); // this call will trigger second action callback
}
