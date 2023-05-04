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

#include "statistics/normal_average.hpp"

#include <gmock/gmock.h>

namespace nodemanager
{

constexpr double doubleNan = std::numeric_limits<double>::quiet_NaN();
using namespace std::chrono_literals;

class NormalAverageParameters
{
  public:
    NormalAverageParameters& sample(double value,
                                    std::chrono::milliseconds time)
    {
        samples_.emplace_back(value, time);
        return *this;
    }

    const std::vector<std::pair<double, std::chrono::milliseconds>>&
        samples() const
    {
        return samples_;
    }

    NormalAverageParameters& expectedAverage(double value)
    {
        expectedAverage_ = value;
        return *this;
    }

    double expectedAverage() const
    {
        return expectedAverage_;
    }

    NormalAverageParameters& expectedMin(double value)
    {
        expectedMin_ = value;
        return *this;
    }

    double expectedMin() const
    {
        return expectedMin_;
    }

    NormalAverageParameters& expectedMax(double value)
    {
        expectedMax_ = value;
        return *this;
    }

    double expectedMax() const
    {
        return expectedMax_;
    }

    NormalAverageParameters& expectedDuration(DurationMs value)
    {
        expectedDuration_ = value;
        return *this;
    }

    DurationMs expectedDuration() const
    {
        return expectedDuration_;
    }

  private:
    std::vector<std::pair<double, std::chrono::milliseconds>> samples_;
    double expectedAverage_ = doubleNan;
    double expectedMin_ = doubleNan;
    double expectedMax_ = doubleNan;
    DurationMs expectedDuration_ = {};
};

struct NormalAverageTest : public testing::Test
{
    NormalAverage sut;
};

TEST_F(NormalAverageTest, hasNoValuesAfterReset)
{
    sut.addSample(10.);
    Clock::stepMs(10);
    sut.reset();

    EXPECT_THAT(sut.getAvg(), testing::NanSensitiveDoubleEq(doubleNan));
    EXPECT_THAT(sut.getMin(), testing::NanSensitiveDoubleEq(doubleNan));
    EXPECT_THAT(sut.getMax(), testing::NanSensitiveDoubleEq(doubleNan));
    EXPECT_THAT(sut.getStatisticsReportingPeriod(), testing::Eq(0ms));
}

struct NormalAverageTestWithParams
    : public NormalAverageTest,
      public testing::WithParamInterface<NormalAverageParameters>
{
    void SetUp() override
    {
        for (auto [sample, time] : GetParam().samples())
        {
            sut.addSample(sample);
            Clock::stepMs(time.count());
        }
    }
};

INSTANTIATE_TEST_SUITE_P(WithNoSamples, NormalAverageTestWithParams,
                         ::testing::Values(NormalAverageParameters()
                                               .expectedAverage(doubleNan)
                                               .expectedMin(doubleNan)
                                               .expectedMax(doubleNan)
                                               .expectedDuration(0ms)));

INSTANTIATE_TEST_SUITE_P(WithInvalidSample, NormalAverageTestWithParams,
                         ::testing::Values(NormalAverageParameters()
                                               .sample(10., 10ms)
                                               .sample(doubleNan, 10ms)
                                               .expectedAverage(doubleNan)
                                               .expectedMin(doubleNan)
                                               .expectedMax(doubleNan)
                                               .expectedDuration(0ms),
                                           NormalAverageParameters()
                                               .sample(5., 10ms)
                                               .sample(doubleNan, 10ms)
                                               .sample(10., 10ms)
                                               .expectedAverage(10.)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(10ms)));

INSTANTIATE_TEST_SUITE_P(WithDeltaTime0ms, NormalAverageTestWithParams,
                         ::testing::Values(NormalAverageParameters()
                                               .sample(10., 0ms)
                                               .expectedAverage(doubleNan)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(0ms),
                                           NormalAverageParameters()
                                               .sample(10., 10ms)
                                               .sample(1000., 0ms)
                                               .sample(25., 20ms)
                                               .expectedAverage(20.)
                                               .expectedMin(10.)
                                               .expectedMax(1000.)
                                               .expectedDuration(30ms)));

INSTANTIATE_TEST_SUITE_P(WithValidSamples, NormalAverageTestWithParams,
                         ::testing::Values(NormalAverageParameters()
                                               .sample(10., 10ms)
                                               .expectedAverage(10.)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(10ms),
                                           NormalAverageParameters()
                                               .sample(6., 10ms)
                                               .sample(30., 20ms)
                                               .expectedAverage(22.)
                                               .expectedMin(6.)
                                               .expectedMax(30.)
                                               .expectedDuration(30ms),
                                           NormalAverageParameters()
                                               .sample(6.5, 10ms)
                                               .sample(3.5, 10ms)
                                               .expectedAverage(5.)
                                               .expectedMin(3.5)
                                               .expectedMax(6.5)
                                               .expectedDuration(20ms)));

TEST_P(NormalAverageTestWithParams, calculatesAverage)
{
    EXPECT_THAT(sut.getAvg(),
                testing::NanSensitiveDoubleEq(GetParam().expectedAverage()));
}

TEST_P(NormalAverageTestWithParams, calculatesMinValue)
{
    EXPECT_THAT(sut.getMin(),
                testing::NanSensitiveDoubleEq(GetParam().expectedMin()));
}

TEST_P(NormalAverageTestWithParams, calculatesMaxValue)
{
    EXPECT_THAT(sut.getMax(),
                testing::NanSensitiveDoubleEq(GetParam().expectedMax()));
}

TEST_P(NormalAverageTestWithParams, calculatedStatisticReportingPeriod)
{
    EXPECT_THAT(sut.getStatisticsReportingPeriod().count(),
                testing::Eq(GetParam().expectedDuration().count()));
}

} // namespace nodemanager
