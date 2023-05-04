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

#include "actions/action_cpu_utilization.hpp"
#include "clock.hpp"
#include "mocks/moving_average_mock.hpp"
#include "statistics/average.hpp"

#include <memory>

#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr const double kThresholdValue = 10;
static constexpr const double kBelowThresholdValue = 5;
static constexpr const double kAboveThresholdValue = 15;
static constexpr const double kNoSensorValueOnTestStart =
    std::numeric_limits<double>::quiet_NaN();
static constexpr const double kFakeValue = 0;

class ActionCpuUtilizationTest
{
  protected:
    std::shared_ptr<::testing::NiceMock<MovingAverageMock>> average_ =
        std::make_shared<::testing::NiceMock<MovingAverageMock>>();
    std::shared_ptr<ActionCpuUtilization> sut_ =
        std::make_shared<ActionCpuUtilization>(kThresholdValue, average_);
};
struct ParamBundle
{
    double previousAverage;
    double currentAverage;
    std::optional<TriggerActionType> actionType;
};

class ActionCpuUtilizationAverageTransitionsTest
    : public ActionCpuUtilizationTest,
      public ::testing::TestWithParam<ParamBundle>
{
  protected:
    ParamBundle params = GetParam();
};

INSTANTIATE_TEST_SUITE_P(
    MixedAveragesValues, ActionCpuUtilizationAverageTransitionsTest,
    testing::Values(
        ParamBundle{kBelowThresholdValue, kBelowThresholdValue, std::nullopt},
        ParamBundle{kBelowThresholdValue, kThresholdValue, std::nullopt},
        ParamBundle{kBelowThresholdValue, kAboveThresholdValue,
                    TriggerActionType::deactivate},
        ParamBundle{kThresholdValue, kBelowThresholdValue,
                    TriggerActionType::trigger},
        ParamBundle{kThresholdValue, kThresholdValue, std::nullopt},
        ParamBundle{kThresholdValue, kAboveThresholdValue,
                    TriggerActionType::deactivate},
        ParamBundle{kAboveThresholdValue, kBelowThresholdValue,
                    TriggerActionType::trigger},
        ParamBundle{kAboveThresholdValue, kThresholdValue, std::nullopt},
        ParamBundle{kAboveThresholdValue, kAboveThresholdValue, std::nullopt},
        ParamBundle{kAboveThresholdValue, kAboveThresholdValue, std::nullopt}));

TEST_P(ActionCpuUtilizationAverageTransitionsTest, AllCases)
{
    ON_CALL(*average_, getAvg())
        .WillByDefault(testing::Return(params.previousAverage));
    sut_->updateReading(kFakeValue);
    ON_CALL(*average_, getAvg())
        .WillByDefault(testing::Return(params.currentAverage));
    EXPECT_EQ(sut_->updateReading(kFakeValue), params.actionType);
}

class ActionCpuUtilizationNoSensorValueTest : public ::testing::Test
{
  protected:
    std::shared_ptr<::testing::NiceMock<MovingAverageMock>> average_ =
        std::make_shared<::testing::NiceMock<MovingAverageMock>>();
    std::shared_ptr<ActionCpuUtilization> sut_ =
        std::make_shared<ActionCpuUtilization>(kThresholdValue, average_);
};

TEST_F(ActionCpuUtilizationNoSensorValueTest, AverageNaNValue)
{
    ON_CALL(*average_, getAvg())
        .WillByDefault(testing::Return(kThresholdValue));
    EXPECT_EQ(sut_->updateReading(kThresholdValue), std::nullopt);
}
