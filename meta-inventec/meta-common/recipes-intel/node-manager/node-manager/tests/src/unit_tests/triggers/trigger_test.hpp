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

#include "mocks/action_mock.hpp"
#include "triggers/trigger.hpp"
#include "triggers/trigger_capabilities.hpp"

#include <ctime>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class TriggerTest : public ::testing::Test
{
  protected:
    testing::MockFunction<void(TriggerActionType)> callback_;
    std::shared_ptr<ActionMock> action_ =
        std::make_shared<testing::NiceMock<ActionMock>>();
    std::shared_ptr<Trigger> sut_ =
        std::make_shared<Trigger>(action_, callback_.AsStdFunction());
};

TEST_F(TriggerTest, PassValueToActionObject)
{
    int random = std::rand();
    EXPECT_CALL(*action_, updateReading(random));
    sut_->updateValue(random);
}

TEST_F(TriggerTest, ReportMissingReadingEventCallback)
{
    EXPECT_CALL(callback_, Call(TriggerActionType::missingReading));

    sut_->reportEvent(SensorEventType::readingMissing,
                      {SensorReadingType{}, DeviceIndex{}},
                      {ReadingType{}, DeviceIndex{}});
}

TEST_F(TriggerTest, ReportNonMissingReadingEvent)
{
    std::vector<SensorEventType> events;
    events.push_back(SensorEventType::sensorAppear);
    events.push_back(SensorEventType::sensorDisappear);

    for (auto event : events)
    {
        EXPECT_CALL(callback_, Call(testing::_)).Times(0);
        sut_->reportEvent(event, {SensorReadingType{}, DeviceIndex{}},
                          {ReadingType{}, DeviceIndex{}});
    }
}

TEST_F(TriggerTest, ProperTriggerActionTypeCallback)
{
    std::vector<std::tuple<std::optional<TriggerActionType>, TriggerActionType>>
        datasetVector = {
            std::make_tuple(std::make_optional(TriggerActionType::deactivate),
                            TriggerActionType::deactivate),
            std::make_tuple(std::make_optional(TriggerActionType::trigger),
                            TriggerActionType::trigger),
        };

    for (const auto& datasetTuple : datasetVector)
    {
        auto& [actionType, triggerActionType] = datasetTuple;
        int random = std::rand();
        EXPECT_CALL(*action_, updateReading(random))
            .WillOnce(testing::Return(actionType));
        EXPECT_CALL(callback_, Call(triggerActionType));
        sut_->updateValue(random);
    }
}
