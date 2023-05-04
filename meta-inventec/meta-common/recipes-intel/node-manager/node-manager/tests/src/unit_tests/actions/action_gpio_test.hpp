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

#include "actions/action_gpio.hpp"

#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr double kLowState = 0;
static constexpr double kHighState = 1;

class ActionGpioTest : public ::testing::TestWithParam<bool>
{
  public:
    void SetUp()
    {
        bool triggerOnRisingEdge = GetParam();
        sut_ = std::make_shared<ActionGpio>(triggerOnRisingEdge);
    }

  protected:
    std::shared_ptr<ActionGpio> sut_;
};

INSTANTIATE_TEST_SUITE_P(TriggerOnRisingEdgeOptions, ActionGpioTest,
                         ::testing::Values(true, false));

TEST_P(ActionGpioTest, NonBooleanValueNoneActionExpected)
{
    EXPECT_EQ(sut_->updateReading(2), std::nullopt);
}

TEST_P(ActionGpioTest, LowStateOnceAfterLowStateNoneActionExpected)
{
    sut_->updateReading(kLowState);
    EXPECT_EQ(sut_->updateReading(kLowState), std::nullopt);
}

TEST_P(ActionGpioTest, HighStateAfterHighStateNoneActionExpected)
{
    sut_->updateReading(kHighState);
    EXPECT_EQ(sut_->updateReading(kHighState), std::nullopt);
}

TEST_P(ActionGpioTest, HighStateAfterLowStateActionExpected)
{
    sut_->updateReading(kLowState);
    bool triggerOnRisingEdge = GetParam();
    if (triggerOnRisingEdge)
    {
        EXPECT_EQ(sut_->updateReading(kHighState), TriggerActionType::trigger);
    }
    else
    {
        EXPECT_EQ(sut_->updateReading(kHighState),
                  TriggerActionType::deactivate);
    }
}

TEST_P(ActionGpioTest, LowStateAfterHighStateActionExpected)
{
    sut_->updateReading(kHighState);
    bool triggerOnRisingEdge = GetParam();
    if (triggerOnRisingEdge)
    {
        EXPECT_EQ(sut_->updateReading(kLowState),
                  TriggerActionType::deactivate);
    }
    else
    {
        EXPECT_EQ(sut_->updateReading(kLowState), TriggerActionType::trigger);
    }
}
