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

#include "mocks/accumulator_mock.hpp"
#include "statistics/statistic.hpp"

#include <gmock/gmock.h>

namespace nodemanager
{

struct StatisticTest : public testing::Test
{
    StatisticTest()
    {
        ON_CALL(accumulator, getCurrentValue())
            .WillByDefault(testing::Return(2.3));
        ON_CALL(accumulator, getMax()).WillByDefault(testing::Return(11.3));
        ON_CALL(accumulator, getMin()).WillByDefault(testing::Return(13.1));
        ON_CALL(accumulator, getAvg()).WillByDefault(testing::Return(7.));
        ON_CALL(accumulator, getStatisticsReportingPeriod())
            .WillByDefault(testing::Return(std::chrono::milliseconds(13035)));
    }

    template <class T>
    T getValuesMapItem(const std::string& key) const
    {
        return std::get<T>(sut.getValuesMap().at(key));
    }

    static constexpr double doubleNan =
        std::numeric_limits<double>::quiet_NaN();

    std::string name = "name";
    std::shared_ptr<AccumulatorMock> accumulatorPtr =
        std::make_shared<testing::NiceMock<AccumulatorMock>>();
    AccumulatorMock& accumulator = *accumulatorPtr;
    Statistic sut{name, accumulatorPtr};
};

TEST_F(StatisticTest, Resets)
{
    EXPECT_CALL(accumulator, reset());
    sut.reset();
}

TEST_F(StatisticTest, ReportEventDoesNothing)
{
    sut.reportEvent(SensorEventType{}, {SensorReadingType{}, DeviceIndex{}},
                    {ReadingType{}, DeviceIndex{}});
}

TEST_F(StatisticTest, SampleAddedWhenUpdateValueWithValidValue)
{
    EXPECT_CALL(accumulator, addSample(testing::DoubleEq(1)));
    sut.updateValue(1);
}

TEST_F(StatisticTest, NoSampleAddedWhenUpdatedValueWithNan)
{
    EXPECT_CALL(accumulator, addSample(testing::_)).Times(0);
    sut.updateValue(std::numeric_limits<double>::quiet_NaN());
}

TEST_F(StatisticTest, NoSampleAddedWhenStatisticDisabled)
{
    EXPECT_CALL(accumulator, addSample(testing::_)).Times(0);
    sut.disableStatisticCalculation();
    sut.updateValue(1);
}

struct StatisticTestExpectingEmptyValuesMap
    : public StatisticTest,
      public testing::WithParamInterface<std::function<void(StatisticTest&)>>
{
    void SetUp() override
    {
        GetParam()(*this);
    }
};

INSTANTIATE_TEST_SUITE_P(WithNoSamples, StatisticTestExpectingEmptyValuesMap,
                         testing::Values([](auto& ts) {}));

INSTANTIATE_TEST_SUITE_P(WithResetAfterAddingSample,
                         StatisticTestExpectingEmptyValuesMap,
                         testing::Values([](auto& ts) {
                             ts.sut.updateValue(42.7);
                             ts.sut.reset();
                         }));

INSTANTIATE_TEST_SUITE_P(WithReenableAfterAddingSample,
                         StatisticTestExpectingEmptyValuesMap,
                         testing::Values([](auto& ts) {
                             ts.sut.updateValue(42.7);
                             ts.sut.enableStatisticCalculation();
                         }));

TEST_P(StatisticTestExpectingEmptyValuesMap, ValueMapContainsNanCurrent)
{
    EXPECT_THAT(getValuesMapItem<double>("Current"),
                testing::NanSensitiveDoubleEq(doubleNan));
}

TEST_P(StatisticTestExpectingEmptyValuesMap, ValueMapContainsNanMax)
{
    EXPECT_THAT(getValuesMapItem<double>("Max"),
                testing::NanSensitiveDoubleEq(doubleNan));
}

TEST_P(StatisticTestExpectingEmptyValuesMap, ValueMapContainsNanMin)
{
    EXPECT_THAT(getValuesMapItem<double>("Min"),
                testing::NanSensitiveDoubleEq(doubleNan));
}

TEST_P(StatisticTestExpectingEmptyValuesMap, ValueMapContainsNanAverage)
{
    EXPECT_THAT(getValuesMapItem<double>("Average"),
                testing::NanSensitiveDoubleEq(doubleNan));
}

TEST_P(StatisticTestExpectingEmptyValuesMap,
       ValueMapContainsStatisticsReportingPeriodFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<uint32_t>("StatisticsReportingPeriod"),
                testing::Eq(13u));
}

TEST_P(StatisticTestExpectingEmptyValuesMap,
       ValueMapContainsNanMeasurementState)
{
    EXPECT_THAT(getValuesMapItem<bool>("MeasurementState"), testing::Eq(false));
}

struct StatisticTestExpectingValuesMapBuildFromAccumulator
    : public StatisticTest,
      public testing::WithParamInterface<std::function<void(StatisticTest&)>>
{
    void SetUp() override
    {
        GetParam()(*this);
    }
};

INSTANTIATE_TEST_SUITE_P(WithOneSample,
                         StatisticTestExpectingValuesMapBuildFromAccumulator,
                         testing::Values([](auto& ts) {
                             ts.sut.updateValue(42.7);
                         }));

TEST_P(StatisticTestExpectingValuesMapBuildFromAccumulator,
       ValueMapContainsCurrentFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<double>("Current"), testing::DoubleEq(2.3));
}

TEST_P(StatisticTestExpectingValuesMapBuildFromAccumulator,
       ValueMapContainsMaxFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<double>("Max"), testing::DoubleEq(11.3));
}

TEST_P(StatisticTestExpectingValuesMapBuildFromAccumulator,
       ValueMapContainsMinFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<double>("Min"), testing::DoubleEq(13.1));
}

TEST_P(StatisticTestExpectingValuesMapBuildFromAccumulator,
       ValueMapContainsAverageFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<double>("Average"), testing::DoubleEq(7.));
}

TEST_P(StatisticTestExpectingValuesMapBuildFromAccumulator,
       ValueMapContainsStatisticsReportingPeriodFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<uint32_t>("StatisticsReportingPeriod"),
                testing::Eq(13u));
}

TEST_P(StatisticTestExpectingValuesMapBuildFromAccumulator,
       ValueMapContainsMeasurementFromAccumulator)
{
    EXPECT_THAT(getValuesMapItem<bool>("MeasurementState"), testing::Eq(true));
}

struct StatisticTestExpectingMeasurementStateDisabled
    : public StatisticTest,
      public testing::WithParamInterface<std::function<void(StatisticTest&)>>
{
    void SetUp() override
    {
        GetParam()(*this);
    }
};

INSTANTIATE_TEST_SUITE_P(WithDisableAfterAddingSample,
                         StatisticTestExpectingMeasurementStateDisabled,
                         testing::Values([](auto& ts) {
                             ts.sut.updateValue(42.7);
                             ts.sut.disableStatisticCalculation();
                         }));

INSTANTIATE_TEST_SUITE_P(WithNanSampleAfterGoodSample,
                         StatisticTestExpectingMeasurementStateDisabled,
                         testing::Values([](auto& ts) {
                             ts.sut.updateValue(42.7);
                             ts.sut.updateValue(doubleNan);
                         }));

TEST_P(StatisticTestExpectingMeasurementStateDisabled,
       ValueMapContainsMeasurementFromAccumulatorStatsDisabled)
{
    EXPECT_THAT(getValuesMapItem<bool>("MeasurementState"), testing::Eq(false));
}

} // namespace nodemanager
