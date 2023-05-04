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
#include "control/scalability/cpu_scalability.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class ProportionalCpuScalabilityTest : public ::testing::Test
{
  protected:
    ProportionalCpuScalabilityTest()
    {
    }

    virtual void SetUp() override
    {
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPresence, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&rc_));
        cpuFactor_ = std::make_unique<CpuScalability>(devManMock_, false);
    }

    virtual ~ProportionalCpuScalabilityTest() = default;

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> rc_;
    std::unique_ptr<CpuScalability> cpuFactor_;
};

TEST_F(ProportionalCpuScalabilityTest, UnregisterInDescrutcor)
{
    EXPECT_CALL(*devManMock_,
                unregisterReadingConsumer(testing::Truly(
                    [this](const auto& arg) { return arg == rc_; })));
    cpuFactor_ = nullptr;
}

TEST_F(ProportionalCpuScalabilityTest, DefaultsNoUpdateNoCpus)
{
    const auto ret = cpuFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));
}

TEST_F(ProportionalCpuScalabilityTest, OnlyCpu0isActive)
{
    std::bitset<8> bs(1);
    rc_->updateValue(static_cast<double>(bs.to_ulong()));
    const auto ret = cpuFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(1, 0, 0, 0, 0, 0, 0, 0));
}

TEST_F(ProportionalCpuScalabilityTest, OnlyCpu2isActive)
{
    std::bitset<8> bs(4);
    rc_->updateValue(static_cast<double>(bs.to_ulong()));
    const auto ret = cpuFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0, 0, 1, 0, 0, 0, 0, 0));
}

TEST_F(ProportionalCpuScalabilityTest, Cpu0And2AreActive)
{
    std::bitset<8> bs(5);
    rc_->updateValue(static_cast<double>(bs.to_ulong()));
    const auto ret = cpuFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0.5, 0, 0.5, 0, 0, 0, 0, 0));
}

TEST_F(ProportionalCpuScalabilityTest, AllCpusActive)
{
    std::bitset<8> bs(255);
    rc_->updateValue(static_cast<double>(bs.to_ulong()));
    const auto ret = cpuFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0.125, 0.125, 0.125, 0.125, 0.125,
                                          0.125, 0.125, 0.125));
}

TEST_F(ProportionalCpuScalabilityTest, MoreCpuThanExpected)
{
    rc_->updateValue(256.0);
    const auto ret = cpuFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));
}
