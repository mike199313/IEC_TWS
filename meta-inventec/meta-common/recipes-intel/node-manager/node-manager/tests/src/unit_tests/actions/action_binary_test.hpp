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

#include "actions/action_binary.hpp"
#include "clock.hpp"

#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr double kInitialHostResetValue = 1;
static constexpr double kNoHostReset = 0;
static constexpr double kHostReset = 1;

class ActionBinaryTest : public ::testing::Test
{
  protected:
    std::shared_ptr<ActionBinary> sut_ =
        std::make_shared<ActionBinary>(kInitialHostResetValue);
    std::shared_ptr<Clock> clock_ = std::make_shared<Clock>();
};

TEST_F(ActionBinaryTest, NonBooleanValueNoneActionExpected)
{
    EXPECT_EQ(sut_->updateReading(2), std::nullopt);
}

TEST_F(ActionBinaryTest, NoHostResetOnceAfterNoHostResetNoneActionExpected)
{
    sut_->updateReading(kNoHostReset);
    EXPECT_EQ(sut_->updateReading(kNoHostReset), std::nullopt);
}

TEST_F(ActionBinaryTest,
       HostResetOnceAfterNoHostResetTriggerTriggeredActionExpected)
{
    sut_->updateReading(kNoHostReset);
    EXPECT_EQ(sut_->updateReading(kHostReset), TriggerActionType::trigger);
}

TEST_F(ActionBinaryTest, HostResetOnceAfterHostResetNoneActionExpected)
{
    sut_->updateReading(kHostReset);
    EXPECT_EQ(sut_->updateReading(kHostReset), std::nullopt);
}

TEST_F(ActionBinaryTest,
       NoHostResetOnceAfterHostResetTriggerDeactivatedActionExpected)
{
    sut_->updateReading(kHostReset);
    EXPECT_EQ(sut_->updateReading(kNoHostReset), TriggerActionType::deactivate);
}
