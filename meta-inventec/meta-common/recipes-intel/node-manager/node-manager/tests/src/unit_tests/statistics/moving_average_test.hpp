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

#include "statistics/moving_average.hpp"

#include <gmock/gmock.h>

namespace moving_avg
{

constexpr double doubleNan = std::numeric_limits<double>::quiet_NaN();

using namespace std::chrono_literals;

class MovingAverageParameters
{
  public:
    MovingAverageParameters& sample(double value,
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

    MovingAverageParameters& expectedAverage(double value)
    {
        expectedAverage_ = value;
        return *this;
    }

    double expectedAverage() const
    {
        return expectedAverage_;
    }

    MovingAverageParameters& expectedMin(double value)
    {
        expectedMin_ = value;
        return *this;
    }

    double expectedMin() const
    {
        return expectedMin_;
    }

    MovingAverageParameters& expectedMax(double value)
    {
        expectedMax_ = value;
        return *this;
    }

    double expectedMax() const
    {
        return expectedMax_;
    }

    MovingAverageParameters& expectedDuration(DurationMs value)
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

struct MovingAverageTest : public testing::Test
{
    MovingAverageTest() : sut_(DurationMs{std::chrono::milliseconds{300}}){};
    MovingAverage sut_;
};

TEST_F(MovingAverageTest, hasNoValuesAfterReset)
{
    sut_.addSample(10.);
    Clock::stepMs(5);
    sut_.reset();

    EXPECT_THAT(sut_.getAvg(), testing::NanSensitiveDoubleEq(doubleNan));
    EXPECT_THAT(sut_.getMin(), testing::NanSensitiveDoubleEq(doubleNan));
    EXPECT_THAT(sut_.getMax(), testing::NanSensitiveDoubleEq(doubleNan));
    EXPECT_THAT(sut_.getStatisticsReportingPeriod(), testing::Eq(0ms));
}

struct MovingAverageTestWithParams
    : public MovingAverageTest,
      public ::testing::WithParamInterface<MovingAverageParameters>
{
    void SetUp() override
    {
        for (auto [sample, time] : GetParam().samples())
        {
            sut_.addSample(sample);
            Clock::stepMs(time.count());
        }
    }
};

INSTANTIATE_TEST_SUITE_P(WithNoSamples, MovingAverageTestWithParams,
                         ::testing::Values(MovingAverageParameters()
                                               .expectedAverage(doubleNan)
                                               .expectedMin(doubleNan)
                                               .expectedMax(doubleNan)
                                               .expectedDuration(0ms)));

INSTANTIATE_TEST_SUITE_P(InvalidSample, MovingAverageTestWithParams,
                         ::testing::Values(MovingAverageParameters()
                                               .sample(10., 1ms)
                                               .sample(doubleNan, 1ms)
                                               .expectedAverage(doubleNan)
                                               .expectedMin(doubleNan)
                                               .expectedMax(doubleNan)
                                               .expectedDuration(0ms),
                                           MovingAverageParameters()
                                               .sample(10., 25ms)
                                               .sample(doubleNan, 25ms)
                                               .expectedAverage(doubleNan)
                                               .expectedMin(doubleNan)
                                               .expectedMax(doubleNan)
                                               .expectedDuration(0ms),
                                           MovingAverageParameters()
                                               .sample(5., 5ms)
                                               .sample(doubleNan, 5ms)
                                               .sample(10., 5ms)
                                               .expectedAverage(10.)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(5ms),
                                           MovingAverageParameters()
                                               .sample(5., 25ms)
                                               .sample(doubleNan, 25ms)
                                               .sample(10., 25ms)
                                               .expectedAverage(10.)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(25ms)));

INSTANTIATE_TEST_SUITE_P(WithDeltaTime0ms, MovingAverageTestWithParams,
                         ::testing::Values(MovingAverageParameters()
                                               .sample(10., 0ms)
                                               .expectedAverage(doubleNan)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(0ms),
                                           MovingAverageParameters()
                                               .sample(10., 10ms)
                                               .sample(1000., 0ms)
                                               .sample(25., 20ms)
                                               .expectedAverage(20.)
                                               .expectedMin(10.)
                                               .expectedMax(1000.)
                                               .expectedDuration(30ms)));

INSTANTIATE_TEST_SUITE_P(WithValidSamples, MovingAverageTestWithParams,
                         ::testing::Values(MovingAverageParameters()
                                               .sample(10., 10ms)
                                               .expectedAverage(10.)
                                               .expectedMin(10.)
                                               .expectedMax(10.)
                                               .expectedDuration(10ms),
                                           MovingAverageParameters()
                                               .sample(6., 10ms)
                                               .sample(30., 20ms)
                                               .expectedAverage(22.)
                                               .expectedMin(6.)
                                               .expectedMax(30.)
                                               .expectedDuration(30ms),
                                           MovingAverageParameters()
                                               .sample(6.5, 10ms)
                                               .sample(3.5, 10ms)
                                               .expectedAverage(5.)
                                               .expectedMin(3.5)
                                               .expectedMax(6.5)
                                               .expectedDuration(20ms),
                                           MovingAverageParameters()
                                               .sample(6., 3ms)
                                               .sample(8., 3ms)
                                               .sample(6., 3ms)
                                               .sample(8., 3ms)
                                               .sample(6., 3ms)
                                               .sample(8., 3ms)
                                               .sample(6., 3ms)
                                               .sample(8., 3ms)
                                               .expectedAverage(7.)
                                               .expectedMin(6.)
                                               .expectedMax(8.)
                                               .expectedDuration(24ms)));

INSTANTIATE_TEST_SUITE_P(
    SampleCircularArrayTestWithParams, MovingAverageTestWithParams,
    ::testing::Values(MovingAverageParameters()
                          .sample(10., 309ms) // no sample dropped
                          .expectedAverage(10.)
                          .expectedMin(10.)
                          .expectedMax(10.)
                          .expectedDuration(309ms),
                      MovingAverageParameters()
                          .sample(1000., 10ms)
                          .sample(10., 301ms) // single sample dropped
                          .expectedAverage(10.)
                          .expectedMin(10.)
                          .expectedMax(10.)
                          .expectedDuration(301ms),
                      MovingAverageParameters()
                          .sample(10., 10ms)
                          .sample(1000., 301ms) // single sample dropped
                          .expectedAverage(1000.)
                          .expectedMin(1000.)
                          .expectedMax(1000.)
                          .expectedDuration(301ms),
                      MovingAverageParameters()
                          .sample(25., 300ms) // multiple samples dropped
                          .sample(10., 310ms)
                          .expectedAverage(10.)
                          .expectedMin(10.)
                          .expectedMax(10.)
                          .expectedDuration(310ms)));

TEST_P(MovingAverageTestWithParams, calculatesAverage)
{
    EXPECT_THAT(sut_.getAvg(),
                testing::NanSensitiveDoubleEq(GetParam().expectedAverage()));
}

TEST_P(MovingAverageTestWithParams, calculatesMinValue)
{
    EXPECT_THAT(sut_.getMin(),
                testing::NanSensitiveDoubleEq(GetParam().expectedMin()));
}

TEST_P(MovingAverageTestWithParams, calculatesMaxValue)
{
    EXPECT_THAT(sut_.getMax(),
                testing::NanSensitiveDoubleEq(GetParam().expectedMax()));
}

TEST_P(MovingAverageTestWithParams, calculatedStatisticReportingPeriod)
{
    EXPECT_THAT(sut_.getStatisticsReportingPeriod().count(),
                testing::Eq(GetParam().expectedDuration().count()));
}

} // namespace moving_avg
