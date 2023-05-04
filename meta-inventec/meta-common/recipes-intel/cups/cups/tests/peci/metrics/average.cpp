/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2020 Intel Corporation
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you (License). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or
 *  transmit this software or the related documents without
 *  Intel's prior written permission.
 *
 *  This software and the related documents are provided as is,
 *  with no express or implied warranties, other than those
 *  that are expressly stated in the License.
 */

#include "peci/metrics/average.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::testing;
using namespace ::cups;

class AverageTest : public TestWithParam<std::tuple<double, int>>
{
  public:
    double value;
    unsigned numSamples;
    peci::metrics::AverageCounter average;

  protected:
    void SetUp() override
    {
        value = std::get<0>(GetParam());
        numSamples = static_cast<unsigned>(std::get<1>(GetParam()));
        average = peci::metrics::AverageCounter(numSamples);
    }
};

TEST_P(AverageTest, OneSample)
{
    EXPECT_EQ(average.updateAverage(value), value);
    for (unsigned sample = 2; sample <= numSamples; sample++)
        EXPECT_EQ(average.updateAverage(0), value / sample);
}

TEST_P(AverageTest, AllZeros)
{
    for (unsigned sample = 1; sample <= numSamples; sample++)
        EXPECT_EQ(average.updateAverage(0), 0);
}

TEST_P(AverageTest, AllValuesEqual)
{
    for (unsigned sample = 1; sample <= numSamples; sample++)
        EXPECT_EQ(average.updateAverage(value), value);
}

TEST_P(AverageTest, InsertWhenAllZeros)
{
    for (unsigned sample = 1; sample <= numSamples; sample++)
        EXPECT_EQ(average.updateAverage(0), 0);

    for (unsigned sample = 1; sample <= numSamples; sample++)
        EXPECT_EQ(average.updateAverage(value), (value * sample) / numSamples);
}

TEST_P(AverageTest, InsertZerosWhenAllEqual)
{
    for (unsigned sample = 1; sample <= numSamples; sample++)
        EXPECT_EQ(average.updateAverage(value), value);

    for (unsigned sample = numSamples - 1; sample > 0; sample--)
        EXPECT_EQ(average.updateAverage(0), (value * sample) / numSamples);
}

auto params = Combine(Range(0., 100., 0.5), Range(1, 10));
INSTANTIATE_TEST_CASE_P(OneSample, AverageTest, params);
INSTANTIATE_TEST_CASE_P(AllZeros, AverageTest, params);
INSTANTIATE_TEST_CASE_P(AllValuesEqual, AverageTest, params);
INSTANTIATE_TEST_CASE_P(InsertWhenAllZeros, AverageTest, params);
INSTANTIATE_TEST_CASE_P(InsertZerosWhenAllEqual, AverageTest, params);
