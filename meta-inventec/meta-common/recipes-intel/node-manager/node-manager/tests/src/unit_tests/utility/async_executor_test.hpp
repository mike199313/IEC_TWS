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
#include "common_types.hpp"
#include "utility/async_executor.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

class AsyncExecutorTest : public testing::Test
{
  public:
    virtual ~AsyncExecutorTest() = default;

    virtual void SetUp() override
    {
        sut_ = std::make_shared<AsyncExecutor<int, std::string>>(
            DbusEnvironment::getBus(), std::chrono::milliseconds{5});
    }

    virtual void TearDown()
    {
    }

    std::shared_ptr<AsyncExecutor<int, std::string>> sut_;
};

TEST_F(AsyncExecutorTest, TaskTakesLessThanCheckInterval)
{
    testing::MockFunction<std::string(int)> task;
    testing::MockFunction<void(std::string)> completionHandler;

    EXPECT_CALL(task, Call(42)).WillOnce(testing::Return("Ok"));
    EXPECT_CALL(completionHandler, Call("Ok"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete")));

    sut_->schedule(
        0, [&task]() -> std::string { return task.Call(42); },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    DbusEnvironment::waitForFuture("complete");
}

TEST_F(AsyncExecutorTest, MultipleTasksScheduledOnceAndFinished)
{
    constexpr size_t kTaskCount = 100;
    std::vector<testing::MockFunction<std::string(int)>> tasks(kTaskCount);
    std::vector<testing::MockFunction<void(std::string)>> completionHandlers(
        kTaskCount);
    for (size_t i = 0; i < kTaskCount; i++)
    {
        auto& task = tasks[i];
        auto& completionHandler = completionHandlers[i];

        EXPECT_CALL(task, Call(i))
            .WillOnce(testing::Return("Ok" + std::to_string(i)));
        EXPECT_CALL(completionHandler, Call("Ok" + std::to_string(i)))
            .WillOnce(testing::InvokeWithoutArgs(
                DbusEnvironment::setPromise("complete" + std::to_string(i))));

        sut_->schedule(
            i, [&task, i]() -> std::string { return task.Call(i); },
            [&completionHandler](std::string v) { completionHandler.Call(v); });
    }
    DbusEnvironment::waitForAllFutures();
}

TEST_F(AsyncExecutorTest, TwoTasksWhereOneIsDelayed)
{
    testing::MockFunction<std::string(int)> task;
    testing::MockFunction<void(std::string)> completionHandler;

    EXPECT_CALL(task, Call(42)).WillOnce(testing::Return("Ok"));
    EXPECT_CALL(completionHandler, Call("Ok"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete")));

    EXPECT_CALL(task, Call(24)).WillOnce(testing::Return("Ok2"));
    EXPECT_CALL(completionHandler, Call("Ok2"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete2")));

    sut_->schedule(
        0,
        [&task]() -> std::string {
            std::this_thread::sleep_for(std::chrono::milliseconds{15});
            return task.Call(42);
        },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    sut_->schedule(
        1, [&task]() -> std::string { return task.Call(24); },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    DbusEnvironment::waitForAllFutures();
}

TEST_F(AsyncExecutorTest, SingleTaskTakesLongerThanCheckInterval)
{
    testing::MockFunction<std::string(int)> task;
    testing::MockFunction<void(std::string)> completionHandler;

    EXPECT_CALL(task, Call(42)).WillOnce(testing::Return("Ok"));
    EXPECT_CALL(completionHandler, Call("Ok"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete")));

    sut_->schedule(
        0,
        [&task]() -> std::string {
            std::this_thread::sleep_for(std::chrono::milliseconds{15});
            return task.Call(42);
        },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    DbusEnvironment::waitForFuture("complete");
}

TEST_F(AsyncExecutorTest, AnotheScheduleCalledWhenOneIsPendingDoesNothing)
{
    testing::MockFunction<std::string(int)> task;
    testing::MockFunction<void(std::string)> completionHandler;

    EXPECT_CALL(task, Call(42)).WillOnce(testing::Return("Ok"));
    EXPECT_CALL(completionHandler, Call("Ok"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete")));

    EXPECT_CALL(task, Call(24)).Times(0);

    sut_->schedule(
        0,
        [&task]() -> std::string {
            std::this_thread::sleep_for(std::chrono::milliseconds{15});
            return task.Call(42);
        },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    sut_->schedule(
        0, [&task]() -> std::string { return task.Call(24); },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    DbusEnvironment::waitForFuture("complete");
}

TEST_F(AsyncExecutorTest,
       AnotheScheduleCalledWhenOneIsFinishedMockedFunctionCalledTwice)
{
    testing::MockFunction<std::string(int)> task;
    testing::MockFunction<void(std::string)> completionHandler;

    EXPECT_CALL(task, Call(42))
        .WillOnce(testing::Return("Ok"))
        .WillOnce(testing::Return("Ok2"));
    EXPECT_CALL(completionHandler, Call("Ok"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete")));
    EXPECT_CALL(completionHandler, Call("Ok2"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete2")));

    sut_->schedule(
        0, [&task]() -> std::string { return task.Call(42); },
        [&completionHandler](std::string v) { completionHandler.Call(v); });
    DbusEnvironment::waitForFuture("complete");

    sut_->schedule(
        0, [&task]() -> std::string { return task.Call(42); },
        [&completionHandler](std::string v) { completionHandler.Call(v); });

    DbusEnvironment::waitForFuture("complete2");
}

TEST_F(AsyncExecutorTest, NestedCalls)
{
    testing::MockFunction<std::string(int)> task;
    testing::MockFunction<void(std::string)> completionHandler;
    testing::MockFunction<std::string(int)> task2;
    testing::MockFunction<void(std::string)> completionHandler2;

    EXPECT_CALL(task, Call(42)).WillOnce(testing::Return("Ok"));
    EXPECT_CALL(task2, Call(24)).WillOnce(testing::Return("Ok2"));
    EXPECT_CALL(completionHandler, Call("Ok"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete")));
    EXPECT_CALL(completionHandler2, Call("Ok2"))
        .WillOnce(testing::InvokeWithoutArgs(
            DbusEnvironment::setPromise("complete2")));

    sut_->schedule(
        0, [&task]() -> std::string { return task.Call(42); },
        [&completionHandler, &task2, &completionHandler2, this](std::string v) {
            sut_->schedule(
                0, [&task2]() -> std::string { return task2.Call(24); },
                [&completionHandler2](std::string v) {
                    completionHandler2.Call(v);
                });

            completionHandler.Call(v);
        });
    DbusEnvironment::waitForFuture("complete");
    DbusEnvironment::waitForFuture("complete2");
}

} // namespace nodemanager