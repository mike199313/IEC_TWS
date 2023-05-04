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
#include "peci/metrics/utilization.hpp"
#include "peci/mocks.hpp"
#include "utils/traits.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::testing;
using namespace ::cups;

class ImplTest : public ::testing::TestWithParam<double>
{
  public:
    static constexpr double delay = 0.05;
    static constexpr double margin = 0.5;

  protected:
    void SetUp() override
    {
        Impl::mock = std::make_unique<::testing::NiceMock<Impl::Mock>>();
    }

    void TearDown() override
    {
        Impl::mock.reset();
    }
};

TEST_P(ImplTest, utilizationPercent)
{
    static constexpr uint64_t testMaxUtil = 1'000'000;
    double expectedResult = GetParam();
    uint64_t testUtil =
        static_cast<uint64_t>(testMaxUtil * (expectedResult / 100) * delay);

    peci::metrics::UtilizationCounter<
        Impl, std::numeric_limits<decltype(testMaxUtil)>().max()>
        utilCounter(std::move(Impl()));
    ON_CALL(*Impl::mock, getMaxUtil()).WillByDefault(Return(testMaxUtil));
    ON_CALL(*Impl::mock, probe()).WillByDefault(Return(0));

    // First reading always fails due to lack of previous reading
    auto ret = utilCounter.probe();
    EXPECT_FALSE(ret);
    // TODO make time mockable
    std::this_thread::sleep_for(std::chrono::duration<double>(delay));
    ON_CALL(*Impl::mock, probe()).WillByDefault(Return(testUtil));

    ret = utilCounter.probe();
    ASSERT_TRUE(ret);

    const auto& [delta, maxUtil] = *ret;
    auto result = peci::metrics::convertToPercent(delta, maxUtil);
    EXPECT_LE(result, expectedResult + margin);
    EXPECT_GE(result, expectedResult - margin);
}

INSTANTIATE_TEST_CASE_P(utilizationPercent, ImplTest, Range(0., 100., 0.5));

TEST_F(ImplTest, utilizationOverflow)
{
    static constexpr uint8_t testMaxUtil = utils::bitset_max<8>::value;
    double expectedResult = 99.5;
    uint64_t utilBefore = 1;
    uint64_t utilAfter = 0;

    peci::metrics::UtilizationCounter<Impl, testMaxUtil> utilCounter(
        std::move(Impl()));
    ON_CALL(*Impl::mock, getMaxUtil()).WillByDefault(Return(testMaxUtil));
    ON_CALL(*Impl::mock, probe()).WillByDefault(Return(utilBefore));

    // First reading always fails due to lack of previous reading
    auto ret = utilCounter.probe();
    EXPECT_FALSE(ret);
    // TODO make time mockable
    std::this_thread::sleep_for(std::chrono::duration<double>(1));
    ON_CALL(*Impl::mock, probe()).WillByDefault(Return(utilAfter));

    ret = utilCounter.probe();
    ASSERT_TRUE(ret);

    const auto& [delta, maxUtil] = *ret;
    auto result = peci::metrics::convertToPercent(delta, maxUtil);
    EXPECT_LE(result, expectedResult + margin);
    EXPECT_GE(result, expectedResult - margin);
}
