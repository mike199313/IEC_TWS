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
#include "control/control.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class ControlTest : public ::testing::Test
{
  public:
    ControlTest()
    {
    }

  protected:
    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();

    std::unique_ptr<Control> sut_ = std::make_unique<Control>(devManMock_);
};

TEST_F(ControlTest, CreateControlExpectNoThrow)
{
    EXPECT_NO_THROW(std::make_unique<Control>(devManMock_));
}