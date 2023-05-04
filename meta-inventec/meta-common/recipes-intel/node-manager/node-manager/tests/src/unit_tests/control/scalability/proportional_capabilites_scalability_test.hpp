/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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
#include "control/scalability/proportional_capabilites_scalability.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class ProportionalPcieScalabilityTest : public ::testing::Test
{
  public:
    double nanValue = std::numeric_limits<double>::quiet_NaN();
    void prepareReadingValues(std::vector<double> values)
    {
        for (size_t i = 0; i < kMaxCpuNumber; ++i)
        {
            rcs_[i]->updateValue(values[i]);
        }
    }

  protected:
    ProportionalPcieScalabilityTest()
    {
    }

    virtual void SetUp() override
    {
        rcs_ = std::vector<std::shared_ptr<ReadingConsumer>>(kMaxCpuNumber);
        for (size_t i = 0; i < kMaxCpuNumber; ++i)
        {
            ON_CALL(*devManMock_,
                    registerReadingConsumerHelper(
                        testing::_, ReadingType::pciePowerCapabilitiesMax,
                        static_cast<DeviceIndex>(i)))
                .WillByDefault(testing::SaveArg<0>(&rcs_[i]));
        }
        pcieFactor_ =
            std::make_unique<ProportionalPCieScalability>(devManMock_);
    }

    virtual ~ProportionalPcieScalabilityTest() = default;

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::vector<std::shared_ptr<ReadingConsumer>> rcs_;
    std::unique_ptr<ProportionalPCieScalability> pcieFactor_;
};

TEST_F(ProportionalPcieScalabilityTest, UnregisterInDescrutcor1)
{
    for (size_t i = 0; i < kMaxCpuNumber; ++i)
    {
        EXPECT_CALL(*devManMock_, unregisterReadingConsumer(testing::Truly(
                                      [this, i](const auto& arg) {
                                          return arg.get() == rcs_[i].get();
                                      })));
    }
    pcieFactor_ = nullptr;
}

TEST_F(ProportionalPcieScalabilityTest, DefaultsNoUpdateNoCpus)
{
    const auto ret = pcieFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0, 0, 0, 0, 0, 0, 0, 0));
}

TEST_F(ProportionalPcieScalabilityTest, OnlyCpu0isActive)
{
    std::vector<double> reads{5.0,      nanValue, nanValue, nanValue,
                              nanValue, nanValue, nanValue, nanValue};
    prepareReadingValues(reads);
    const auto ret = pcieFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(1, 0, 0, 0, 0, 0, 0, 0));
}

TEST_F(ProportionalPcieScalabilityTest, OnlyCpu5isActive)
{
    std::vector<double> reads{nanValue, nanValue, nanValue, nanValue,
                              nanValue, 5.0,      nanValue, nanValue};
    prepareReadingValues(reads);
    const auto ret = pcieFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0, 0, 0, 0, 0, 1, 0, 0));
}

TEST_F(ProportionalPcieScalabilityTest,
       AllCpusActiveSameMaxCapsExpectEqualDistribution)
{
    std::vector<double> reads{5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
    prepareReadingValues(reads);
    const auto ret = pcieFactor_->getFactors();
    EXPECT_THAT(ret, testing::ElementsAre(0.125, 0.125, 0.125, 0.125, 0.125,
                                          0.125, 0.125, 0.125));
}

TEST_F(ProportionalPcieScalabilityTest,
       UnequalMaxDistributionExecptCorrectScalability)
{
    std::vector<double> reads{5.0,      15.0,     nanValue, nanValue,
                              nanValue, nanValue, nanValue, nanValue};
    prepareReadingValues(reads);
    const auto ret = pcieFactor_->getFactors();
    EXPECT_THAT(ret,
                testing::ElementsAre(0.25, 0.75, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}