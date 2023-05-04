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

#include "actions/action.hpp"
#include "clock.hpp"

#include <limits>

#include <gtest/gtest.h>

using namespace nodemanager;

TEST(ActionTest, TriggerDeactivatedActionDouble)
{
    std::shared_ptr<Action> sut_ = std::make_shared<Action>(0.5);

    EXPECT_EQ(sut_->updateReading(0.4), TriggerActionType::deactivate);
}

TEST(ActionTest, TriggerTriggeredActionDouble)
{
    std::shared_ptr<Action> sut_ = std::make_shared<Action>(0.5);

    EXPECT_EQ(sut_->updateReading(0.6), TriggerActionType::trigger);
}

TEST(ActionTest, NoActionTriggeredWhenStableDouble)
{
    std::shared_ptr<Action> sut_ = std::make_shared<Action>(0.5);

    EXPECT_EQ(sut_->updateReading(0.5), std::nullopt);
}
