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
#include "readings/reading_consumer.hpp"
#include "sensors/sensor_reading.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class SensorReadingTest : public ::testing::Test
{
  protected:
    SensorReadingTest()
    {
    }

    virtual ~SensorReadingTest() = default;

    virtual void SetUp() override
    {
        sut_ = std::make_shared<SensorReading>(
            SensorReadingType::acPlatformPower, deviceIndex_,
            eventCallback_.AsStdFunction());
    }

    testing::NiceMock<testing::MockFunction<void(
        SensorEventType eventType,
        std::shared_ptr<SensorReadingIf> sensorReading)>>
        eventCallback_;
    std::shared_ptr<SensorReadingIf> sut_;
    DeviceIndex deviceIndex_ = 1;
};

struct SensorStatusEventParams
{
    std::optional<SensorReadingStatus> statusAtStart;
    std::vector<std::pair<SensorReadingStatus, unsigned int>> statusRequests;
    std::vector<SensorEventType> expectedEvents;
};

class SensorReadingTestCheckEventsBasedOnStatus
    : public ::testing::WithParamInterface<SensorStatusEventParams>,
      public SensorReadingTest
{
};

INSTANTIATE_TEST_SUITE_P(
    StartFromUninitializedStatus, SensorReadingTestCheckEventsBasedOnStatus,
    ::testing::Values(
        SensorStatusEventParams{std::nullopt,
                                {{SensorReadingStatus::invalid, 1}},
                                {SensorEventType::sensorAppear}},
        SensorStatusEventParams{std::nullopt,
                                {{SensorReadingStatus::invalid, 3}},
                                {SensorEventType::sensorAppear}},
        SensorStatusEventParams{
            std::nullopt,
            {{SensorReadingStatus::valid, 1}},
            {SensorEventType::sensorAppear, SensorEventType::readingAvailable}},
        SensorStatusEventParams{
            std::nullopt,
            {{SensorReadingStatus::valid, 3}},
            {SensorEventType::sensorAppear, SensorEventType::readingAvailable}},
        SensorStatusEventParams{std::nullopt,
                                {{SensorReadingStatus::unavailable, 1}},
                                {SensorEventType::sensorDisappear}},
        SensorStatusEventParams{std::nullopt,
                                {{SensorReadingStatus::unavailable, 3}},
                                {SensorEventType::sensorDisappear}}));

INSTANTIATE_TEST_SUITE_P(
    StartFromValidValueStatus, SensorReadingTestCheckEventsBasedOnStatus,
    ::testing::Values(
        SensorStatusEventParams{SensorReadingStatus::valid,
                                {{SensorReadingStatus::invalid, 1}},
                                {SensorEventType::readingMissing}},
        SensorStatusEventParams{SensorReadingStatus::valid,
                                {{SensorReadingStatus::invalid, 3}},
                                {SensorEventType::readingMissing}},
        SensorStatusEventParams{SensorReadingStatus::valid,
                                {{SensorReadingStatus::unavailable, 1}},
                                {SensorEventType::sensorDisappear,
                                 SensorEventType::readingMissing}},
        SensorStatusEventParams{SensorReadingStatus::valid,
                                {{SensorReadingStatus::unavailable, 3}},
                                {SensorEventType::sensorDisappear,
                                 SensorEventType::readingMissing}},
        SensorStatusEventParams{SensorReadingStatus::valid,
                                {{SensorReadingStatus::valid, 1}},
                                {}}));

INSTANTIATE_TEST_SUITE_P(
    StartFromInvalidValueStatus, SensorReadingTestCheckEventsBasedOnStatus,
    ::testing::Values(
        SensorStatusEventParams{SensorReadingStatus::invalid,
                                {{SensorReadingStatus::valid, 1}},
                                {SensorEventType::readingAvailable}},
        SensorStatusEventParams{SensorReadingStatus::invalid,
                                {{SensorReadingStatus::valid, 3}},
                                {SensorEventType::readingAvailable}},
        SensorStatusEventParams{SensorReadingStatus::invalid,
                                {{SensorReadingStatus::unavailable, 1}},
                                {SensorEventType::sensorDisappear}},
        SensorStatusEventParams{SensorReadingStatus::invalid,
                                {{SensorReadingStatus::unavailable, 3}},
                                {SensorEventType::sensorDisappear}},
        SensorStatusEventParams{SensorReadingStatus::invalid,
                                {{SensorReadingStatus::invalid, 1}},
                                {}}));

INSTANTIATE_TEST_SUITE_P(
    StartFromUnavailableStatus, SensorReadingTestCheckEventsBasedOnStatus,
    ::testing::Values(
        SensorStatusEventParams{
            SensorReadingStatus::unavailable,
            {{SensorReadingStatus::valid, 1}},
            {SensorEventType::sensorAppear, SensorEventType::readingAvailable}},
        SensorStatusEventParams{
            SensorReadingStatus::unavailable,
            {{SensorReadingStatus::valid, 3}},
            {SensorEventType::sensorAppear, SensorEventType::readingAvailable}},
        SensorStatusEventParams{SensorReadingStatus::unavailable,
                                {{SensorReadingStatus::invalid, 1}},
                                {SensorEventType::sensorAppear}},
        SensorStatusEventParams{SensorReadingStatus::unavailable,
                                {{SensorReadingStatus::invalid, 3}},
                                {SensorEventType::sensorAppear}},
        SensorStatusEventParams{SensorReadingStatus::unavailable,
                                {{SensorReadingStatus::unavailable, 1}},
                                {}}));

INSTANTIATE_TEST_SUITE_P(
    MultipleStatusRequestsExpectMultipleEvents,
    SensorReadingTestCheckEventsBasedOnStatus,
    ::testing::Values(
        SensorStatusEventParams{
            std::nullopt,
            {{SensorReadingStatus::invalid, 1},
             {SensorReadingStatus::unavailable, 1},
             {SensorReadingStatus::invalid, 1},
             {SensorReadingStatus::unavailable, 1},
             {SensorReadingStatus::invalid, 1},
             {SensorReadingStatus::unavailable, 1}},
            {SensorEventType::sensorAppear, SensorEventType::sensorDisappear,
             SensorEventType::sensorAppear, SensorEventType::sensorDisappear,
             SensorEventType::sensorAppear, SensorEventType::sensorDisappear}},
        SensorStatusEventParams{
            std::nullopt,
            {{SensorReadingStatus::unavailable, 1},
             {SensorReadingStatus::invalid, 1},
             {SensorReadingStatus::valid, 1},
             {SensorReadingStatus::invalid, 1},
             {SensorReadingStatus::unavailable, 1}},
            {SensorEventType::sensorDisappear, SensorEventType::sensorAppear,
             SensorEventType::readingAvailable, SensorEventType::readingMissing,
             SensorEventType::sensorDisappear}},
        SensorStatusEventParams{
            std::nullopt,
            {{SensorReadingStatus::valid, 1},
             {SensorReadingStatus::unavailable, 1},
             {SensorReadingStatus::invalid, 1}},
            {SensorEventType::sensorAppear, SensorEventType::readingAvailable,
             SensorEventType::sensorDisappear, SensorEventType::readingMissing,
             SensorEventType::sensorAppear}}));

TEST_P(SensorReadingTestCheckEventsBasedOnStatus,
       ExpectCorrectEventsWhenSpecifiedStatusComes)
{
    ::testing::InSequence forceExpectationsSequence;
    auto [statusAtStart, statusRequests, expectedEvents] = GetParam();
    if (statusAtStart)
    {
        sut_->setStatus(*statusAtStart);
    }
    for (auto event : expectedEvents)
    {
        EXPECT_CALL(eventCallback_,
                    Call(event, testing::Pointee(testing::Ref(*sut_))));
    }
    if (0 == expectedEvents.size())
    {
        EXPECT_CALL(eventCallback_,
                    Call(testing::_, testing::Pointee(testing::Ref(*sut_))))
            .Times(0);
    }
    for (auto [status, callsNumber] : statusRequests)
    {
        for (unsigned int index = 0; index < callsNumber; index++)
        {
            sut_->setStatus(status);
        }
    }
}

TEST_P(SensorReadingTestCheckEventsBasedOnStatus,
       NoCallbackNoExceptionAndNoEventNotification)
{
    auto [statusAtStart, statusRequests, expectedEvents] = GetParam();
    sut_ = std::make_unique<SensorReading>(SensorReadingType::acPlatformPower,
                                           deviceIndex_);
    if (statusAtStart)
    {
        sut_->setStatus(*statusAtStart);
    }
    EXPECT_CALL(eventCallback_, Call(testing::_, testing::_)).Times(0);
    for (auto [status, callsNumber] : statusRequests)
    {
        for (unsigned int index = 0; index < callsNumber; index++)
        {
            ASSERT_NO_THROW(sut_->setStatus(status));
        }
    }
}

TEST_F(SensorReadingTest, HealthOkWhenStatusNotSet)
{
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(SensorReadingTest, HealthOkWhenStatusUnavailable)
{
    sut_->setStatus(SensorReadingStatus::unavailable);
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(SensorReadingTest, HealthOkWhenStatusValid)
{
    sut_->setStatus(SensorReadingStatus::valid);
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(SensorReadingTest, HealthWarningWhenStatusInvalid)
{
    sut_->setStatus(SensorReadingStatus::invalid);
    EXPECT_EQ(sut_->getHealth(), NmHealth::warning);
}