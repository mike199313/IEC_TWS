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

#include "budgeting/efficiency_helper.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static const std::chrono::milliseconds kAveragingWindow{3};

class EfficiencyHelperTest : public testing::Test
{
  protected:
    EfficiencyHelperTest() = default;
    virtual ~EfficiencyHelperTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuEfficiency, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&readingCpuEfficiency_));

        sut_ = std::make_shared<EfficiencyHelper>(
            devManMock_, ReadingType::cpuEfficiency, kAveragingWindow);
    };

    std::shared_ptr<EfficiencyHelper> sut_;
    std::shared_ptr<ReadingConsumer> readingCpuEfficiency_;
    std::shared_ptr<testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
};

struct SamplesAndHint
{
    std::vector<double> samples;
    double hint;
};

class EfficiencyHelperTestWithParams
    : public EfficiencyHelperTest,
      public testing::WithParamInterface<SamplesAndHint>
{
};

INSTANTIATE_TEST_SUITE_P(
    ZeroHintExpected, EfficiencyHelperTestWithParams,
    ::testing::Values(
        SamplesAndHint{{}, 0}, SamplesAndHint{{1.0}, 0},
        SamplesAndHint{{-1.0}, 0},
        SamplesAndHint{{1.0, 1.0, std::numeric_limits<double>::quiet_NaN()}, 0},
        SamplesAndHint{
            {1.0, 1.0, std::numeric_limits<double>::quiet_NaN(), 2.0}, 0}));

INSTANTIATE_TEST_SUITE_P(
    PositiveHintExpected, EfficiencyHelperTestWithParams,
    ::testing::Values(SamplesAndHint{{1.0, 1.0}, 1.0},
                      SamplesAndHint{{1.0, 1.0, 2.0}, 1.0},
                      SamplesAndHint{{-1.0, -1.0}, 1.0},
                      SamplesAndHint{{1.0, 1.0, 4.0, 4.1}, 1.0},
                      SamplesAndHint{{1.0, 1.0, 4.0, 1.0, 1.0, 1.0, 4.2},
                                     1.0}));

INSTANTIATE_TEST_SUITE_P(
    NegativeHintExpected, EfficiencyHelperTestWithParams,
    ::testing::Values(SamplesAndHint{{1.0, 1.0, 1.0}, -1.0},
                      SamplesAndHint{{1.0, 1.0, 4.0, 1.0}, -1.0},
                      SamplesAndHint{{1.0, 1.0, 4.0, 3.9}, -1.0},
                      SamplesAndHint{{1.0, 1.0, 4.0, 4.0, 4.0, 6.7}, -1.0},
                      SamplesAndHint{{1.0, 1.0, 4.0, 1.0, 1.0, 1.0, 3.7},
                                     -1.0}));

TEST_P(EfficiencyHelperTestWithParams, ExpectCorrectHint)
{
    auto [samples, hint] = GetParam();
    for (const auto& sample : samples)
    {
        sut_->getHint();
        readingCpuEfficiency_->updateValue(sample);
        Clock::stepMs(1);
    }
    EXPECT_EQ(sut_->getHint(), hint);
}