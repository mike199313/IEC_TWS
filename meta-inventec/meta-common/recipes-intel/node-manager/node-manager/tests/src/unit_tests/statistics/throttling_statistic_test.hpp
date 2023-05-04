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

#include "domains/domain_types.hpp"
#include "mocks/accumulator_mock.hpp"
#include "mocks/domain_capabilities_mock.hpp"
#include "statistics/throttling_statistic.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

struct ThrottlingStatisticTest : public testing::Test
{
    ThrottlingStatisticTest()
    {
        ON_CALL(*this->caps, getMax()).WillByDefault(testing::Return(200));
        ON_CALL(*this->caps, getMin()).WillByDefault(testing::Return(100));
    }

    std::string name = "name";
    std::unique_ptr<AccumulatorMock> accumulatorPtr =
        std::make_unique<testing::NiceMock<AccumulatorMock>>();
    AccumulatorMock& accumulator = *accumulatorPtr;
    std::shared_ptr<DomainCapabilitiesMock> caps =
        std::make_shared<testing::NiceMock<DomainCapabilitiesMock>>();

    ThrottlingStatistic sut{name, std::move(accumulatorPtr), caps};
};

TEST_F(ThrottlingStatisticTest,
       UpdateValueCalculateThrottlingExpectNoThrottling)
{
    EXPECT_CALL(accumulator, addSample(testing::DoubleEq(0)));
    sut.updateValue(200);
}

TEST_F(ThrottlingStatisticTest,
       UpdateValueCalculateThrottlingExpectMaxThrottling)
{
    EXPECT_CALL(accumulator, addSample(testing::DoubleEq(100)));
    sut.updateValue(0.0);
}

TEST_F(ThrottlingStatisticTest,
       UpdateValueCalculateThrottlingExpectCorrectCalulatedValue)
{
    EXPECT_CALL(accumulator, addSample(testing::DoubleEq(50)));
    sut.updateValue(150.0);
}

TEST_F(ThrottlingStatisticTest,
       UpdateValueWithMinHtMaxCalculateThrottlingExpectNoCall)
{
    ON_CALL(*this->caps, getMin()).WillByDefault(testing::Return(210.0));
    EXPECT_CALL(accumulator, addSample(testing::_)).Times(0);
    sut.updateValue(150.0);
}

TEST_F(ThrottlingStatisticTest,
       UpdateValueCalculateThrottlingExpectNaNThrottling)
{
    EXPECT_CALL(accumulator, addSample(testing::_)).Times(0);
    sut.updateValue(std::numeric_limits<double>::quiet_NaN());
}
