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
#include "policies/limit_exception_handler.hpp"
#include "stubs/dbus_chassis_state_stub.hpp"
#include "stubs/dbus_host_state_stub.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class LimitExceptionHandlerTest : public ::testing::Test
{
  protected:
    LimitExceptionHandlerTest() = default;

    virtual ~LimitExceptionHandlerTest() = default;

    virtual void SetUp() override
    {
        LimitExceptionHandleDbusConfig config;
        config.hostStateServiceName = DbusEnvironment::serviceName();
        config.chassisStateServiceName = DbusEnvironment::serviceName();
        config.hostStateObjectPath = dbusHostState_->path();
        config.chassisStateObjectPath = dbusChassisState_->path();
        sut_ = std::make_shared<LimitExceptionHandler>(
            "/Domain/0/Policy/0", config, DbusEnvironment::getBus());
    }

    virtual void TearDown() override
    {
        ASSERT_TRUE(DbusEnvironment::waitForAllFutures());
        sut_ = nullptr;
        ASSERT_TRUE(DbusEnvironment::waitForAllFutures());
    }

    static std::unique_ptr<DbusHostStateStub> makeDbusHostState()
    {
        return std::make_unique<DbusHostStateStub>(
            DbusEnvironment::getIoc(), DbusEnvironment::getBus(),
            DbusEnvironment::getObjServer());
    }
    static std::unique_ptr<DbusChassisStateStub> makeDbusChassis()
    {
        return std::make_unique<DbusChassisStateStub>(
            DbusEnvironment::getIoc(), DbusEnvironment::getBus(),
            DbusEnvironment::getObjServer());
    }
    std::unique_ptr<DbusHostStateStub> dbusHostState_ = makeDbusHostState();
    std::unique_ptr<DbusChassisStateStub> dbusChassisState_ = makeDbusChassis();
    testing::NiceMock<testing::MockFunction<void(boost::system::errc::errc_t)>>
        completionCallback_;

    std::shared_ptr<LimitExceptionHandler> sut_;

    template <class T = void>
    auto throwSdBusError(const std::errc err, const std::string msg)
    {
        return testing::InvokeWithoutArgs([err, msg]() -> T {
            throw sdbusplus::exception::SdBusError(static_cast<int>(err),
                                                   msg.c_str());
            return T{};
        });
    }

    void expectSetHostStateOffWithSuccess()
    {
        EXPECT_CALL(dbusHostState_->requestedHostTransition,
                    setValue("xyz.openbmc_project.State.Host.Transition.Off"))
            .WillOnce(testing::DoAll(
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                testing::Return(true)));
    }

    void expectSetHostStateOffWithError(const std::errc err)
    {
        EXPECT_CALL(dbusHostState_->requestedHostTransition,
                    setValue("xyz.openbmc_project.State.Host.Transition.Off"))
            .WillOnce(testing::DoAll(
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                throwSdBusError<bool>(err, "Error: operation not supported")));
    }

    void expectGetChassisPowerStateOffSuccess()
    {
        EXPECT_CALL(dbusChassisState_->currentPowerState, getValue())
            .WillOnce(testing::DoAll(
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                testing::Return(
                    "xyz.openbmc_project.State.Chassis.PowerState.Off")));
    }

    void expectSetChassisStateOffSuccess()
    {
        EXPECT_CALL(
            dbusChassisState_->requestedPowerTransition,
            setValue("xyz.openbmc_project.State.Chassis.Transition.Off"))
            .WillOnce(testing::DoAll(
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                testing::Return(true)));
    }

    void expectSetChassisStateOffWithError(const std::errc err)
    {
        EXPECT_CALL(
            dbusChassisState_->requestedPowerTransition,
            setValue("xyz.openbmc_project.State.Chassis.Transition.Off"))
            .WillOnce(testing::DoAll(
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                throwSdBusError<bool>(err, "Error: host unreachable")));
    }

    void expectCompletionCode(const boost::system::errc::errc_t err)
    {
        EXPECT_CALL(completionCallback_, Call(err))
            .WillOnce(testing::InvokeWithoutArgs(
                DbusEnvironment::setPromise("completion_promise_tag")));
    }
};

TEST_F(LimitExceptionHandlerTest, PowerOffSucceedsWithSoftShutdown)
{
    expectSetHostStateOffWithSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerTest,
       PowerOffFailsWithSoftShutdownNotSupportedAndSucceedsWithPowerDown)
{
    expectSetHostStateOffWithError(std::errc::not_supported);
    expectSetChassisStateOffSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerTest,
       PowerOffFailsWithSoftShutdownNotSupportedAndPowerDownNotSupported)
{
    expectSetHostStateOffWithError(std::errc::not_supported);
    expectSetChassisStateOffWithError(std::errc::not_supported);
    expectCompletionCode(boost::system::errc::errc_t::not_supported);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerTest, PowerOffWithSoftShutdownSuccessAfterRepeat)
{
    testing::InSequence forceSequence;

    expectSetHostStateOffWithError(std::errc::host_unreachable);
    expectSetHostStateOffWithSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerTest,
       PowerOffSoftShutdownNotSupportedAndPowerDownSuccessAfterRepeat)
{
    testing::InSequence forceSequence;

    expectSetHostStateOffWithError(std::errc::not_supported);
    expectSetChassisStateOffWithError(std::errc::host_unreachable);
    expectSetChassisStateOffSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(
    LimitExceptionHandlerTest,
    PowerOffSoftShutdownNotSupportedAfterRepeatAndPowerDownSuccessAfterRepeat)
{
    testing::InSequence forceSequence;

    expectSetHostStateOffWithError(std::errc::host_unreachable);
    expectSetHostStateOffWithError(std::errc::not_supported);
    expectSetChassisStateOffWithError(std::errc::host_unreachable);
    expectSetChassisStateOffSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

class LimitExceptionHandlerWithShortTimeoutTest
    : public LimitExceptionHandlerTest
{
  protected:
    LimitExceptionHandlerWithShortTimeoutTest()
    {
        std::cout << "[   INFO   ] This is test for timeout. "
                  << "It may take few seconds..." << std::endl;
    }

    virtual ~LimitExceptionHandlerWithShortTimeoutTest() = default;

    virtual void SetUp() override
    {
        LimitExceptionHandleDbusConfig config;
        config.softShutdownTimeout = std::chrono::seconds{fakeTimeoutValue};
        config.powerDownTimeout = std::chrono::seconds{fakeTimeoutValue};
        config.hostStateServiceName = DbusEnvironment::serviceName();
        config.chassisStateServiceName = DbusEnvironment::serviceName();
        config.hostStateObjectPath = dbusHostState_->path();
        config.chassisStateObjectPath = dbusChassisState_->path();
        sut_ = std::make_unique<LimitExceptionHandler>(
            "/Domain/0/Policy/0", config, DbusEnvironment::getBus());
    }
    static constexpr unsigned fakeTimeoutValue{3};

    void expectSetHostStateWithErrorRepeatedly(const std::errc err,
                                               std::chrono::seconds sleepTime)
    {
        EXPECT_CALL(dbusHostState_->requestedHostTransition,
                    setValue("xyz.openbmc_project.State.Host.Transition.Off"))
            .WillRepeatedly(testing::DoAll(
                testing::InvokeWithoutArgs([sleepTime]() -> void {
                    std::this_thread::sleep_for(sleepTime);
                }),
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                throwSdBusError<bool>(err, "Error: host unreachable")));
    }

    void
        expectSetChassisStateWithErrorRepeatedly(const std::errc err,
                                                 std::chrono::seconds sleepTime)
    {
        EXPECT_CALL(
            dbusChassisState_->requestedPowerTransition,
            setValue("xyz.openbmc_project.State.Chassis.Transition.Off"))
            .WillRepeatedly(testing::DoAll(
                testing::InvokeWithoutArgs([sleepTime]() -> void {
                    std::this_thread::sleep_for(sleepTime);
                }),
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                throwSdBusError<bool>(err, "Error: host unreachable")));
    }

    void expectGetChassisPowerStateOffFailure(std::chrono::seconds sleepTime)
    {
        EXPECT_CALL(dbusChassisState_->currentPowerState, getValue())
            .WillOnce(testing::DoAll(
                testing::InvokeWithoutArgs([sleepTime]() -> void {
                    std::this_thread::sleep_for(sleepTime);
                }),
                testing::InvokeWithoutArgs(
                    DbusEnvironment::setPromise("global_promise_tag")),
                testing::Return(
                    "xyz.openbmc_project.State.Chassis.PowerState.On")));
    }
};

TEST_F(LimitExceptionHandlerWithShortTimeoutTest,
       PowerOffFailsWithSoftShutdownTimeoutAndPowerDownTimeout)
{
    expectSetHostStateWithErrorRepeatedly(std::errc::host_unreachable,
                                          std::chrono::seconds(1));

    expectSetChassisStateWithErrorRepeatedly(std::errc::host_unreachable,
                                             std::chrono::seconds(1));

    expectCompletionCode(boost::system::errc::errc_t::timed_out);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerWithShortTimeoutTest,
       PowerOffSoftShutdownTimeoutAndPowerDownSuccess)
{
    expectSetHostStateWithErrorRepeatedly(std::errc::host_unreachable,
                                          std::chrono::seconds(1));

    expectSetChassisStateOffSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerWithShortTimeoutTest,
       PowerOffSoftShutdownTimeoutAndPowerDownSuccessAfterRepeat)
{
    testing::InSequence forceSequence;

    expectSetHostStateWithErrorRepeatedly(std::errc::host_unreachable,
                                          std::chrono::seconds(1));

    expectSetChassisStateOffWithError(std::errc::host_unreachable);
    expectSetChassisStateOffSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerWithShortTimeoutTest,
       DISABLED_PowerOffSoftShutdownNotSupportedAndPowerDownFailOnTimeout)
{
    testing::InSequence forceSequence;

    expectSetHostStateOffWithError(std::errc::not_supported);

    expectSetChassisStateWithErrorRepeatedly(std::errc::host_unreachable,
                                             std::chrono::seconds(1));
    expectCompletionCode(boost::system::errc::errc_t::timed_out);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerWithShortTimeoutTest,
       PowerOffFailsWithSoftShutdownTimeoutAndPowerNotSupported)
{
    testing::InSequence forceSequence;

    expectSetHostStateWithErrorRepeatedly(std::errc::host_unreachable,
                                          std::chrono::seconds(1));
    expectSetChassisStateOffWithError(std::errc::not_supported);
    expectCompletionCode(boost::system::errc::errc_t::not_supported);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}

TEST_F(LimitExceptionHandlerWithShortTimeoutTest,
       PowerOffSoftShutdownSuccessButChassisStateIsStillOnThenPowerDownSuccess)
{
    testing::InSequence forceSequence;

    expectSetHostStateOffWithSuccess();
    expectGetChassisPowerStateOffFailure(std::chrono::seconds(1));
    expectSetChassisStateOffSuccess();
    expectGetChassisPowerStateOffSuccess();
    expectCompletionCode(boost::system::errc::errc_t::success);

    sut_->doAction(LimitException::powerOff,
                   completionCallback_.AsStdFunction());
}