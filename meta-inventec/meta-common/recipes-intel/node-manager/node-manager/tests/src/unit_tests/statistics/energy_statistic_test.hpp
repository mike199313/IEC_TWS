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

#include "clock.hpp"
#include "statistics/energy_statistic.hpp"
#include "statistics/statistic_if.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace nodemanager
{

static constexpr const auto kName = "Name";

class EnergyStatisticTest : public testing::Test
{
  protected:
    std::shared_ptr<EnergyStatistic> sut_ =
        std::make_shared<EnergyStatistic>(kName);
};

TEST_F(EnergyStatisticTest, NoUpdateExpectZeroSum)
{
    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(static_cast<uint64_t>(0)));
}

TEST_F(EnergyStatisticTest, UpdateNonNanValuesExpectCorrectSum)
{
    sut_->updateValue(10.0);
    Clock::stepMs(100);
    sut_->updateValue(20.0);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(static_cast<uint64_t>((30))));
}

TEST_F(EnergyStatisticTest, UpdateNonNanValuesExpectCorrectTimeIncremented)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);
    Clock::stepMs(2200);
    sut_->updateValue(30.0);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(
        std::get<uint32_t>(statValuesMap.at("StatisticsReportingPeriod")),
        testing::Eq(static_cast<uint32_t>(3)));
}

TEST_F(EnergyStatisticTest, UpdateNanValueExpectCorrectSum)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);
    Clock::stepMs(2200);
    sut_->updateValue(std::numeric_limits<double>::quiet_NaN());

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(static_cast<uint64_t>((30))));
}

TEST_F(EnergyStatisticTest, UpdateNanValueExpectCorrectTimeIncremented)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);
    Clock::stepMs(2200);
    sut_->updateValue(30.0);
    Clock::stepMs(3300);
    sut_->updateValue(std::numeric_limits<double>::quiet_NaN());
    Clock::stepMs(4400);
    sut_->updateValue(std::numeric_limits<double>::quiet_NaN());

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(
        std::get<uint32_t>(statValuesMap.at("StatisticsReportingPeriod")),
        testing::Eq(static_cast<uint32_t>(11)));
}

TEST_F(EnergyStatisticTest, UpdateNanValueAndThenNonNaNExpectCorrectSum)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);
    Clock::stepMs(2200);
    sut_->updateValue(30.0);
    Clock::stepMs(3300);
    sut_->updateValue(std::numeric_limits<double>::quiet_NaN());
    Clock::stepMs(4400);
    sut_->updateValue(40.0);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(static_cast<uint64_t>(100)));
}

TEST_F(EnergyStatisticTest, GetValuesMapWhenDurationIsNotCastSafeExpectNaNValue)
{
    sut_->updateValue(10.0);
    Clock::stepMs(std::numeric_limits<uint32_t>::max() + 1);
    sut_->updateValue(20.0);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(
        std::get<uint32_t>(statValuesMap.at("StatisticsReportingPeriod")),
        std::numeric_limits<uint32_t>::quiet_NaN());
}

TEST_F(EnergyStatisticTest, GetNameExpectSameAsProvidedWhileCreatingObject)
{
    EXPECT_THAT(sut_->getName(), testing::StrCaseEq(kName));
}

TEST_F(EnergyStatisticTest, ResetExpectNanSum)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);

    sut_->reset();

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(static_cast<uint64_t>(0)));
}

TEST_F(EnergyStatisticTest, ResetExpectZeroedDuration)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);

    sut_->reset();

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(
        std::get<uint32_t>(statValuesMap.at("StatisticsReportingPeriod")),
        std::numeric_limits<uint32_t>::min());
}

TEST_F(EnergyStatisticTest, UpdateWithFractionalPartExpectCorrectSum)
{
    sut_->updateValue(20.0002);
    Clock::stepMs(1100);
    sut_->updateValue(10.0009);
    Clock::stepMs(2200);
    sut_->updateValue(11.0008);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(41UL));
}

TEST_F(EnergyStatisticTest,
       UpdateWithFractionalPartExpectCastToIntegralAndAccumulateLetfovers)
{
    sut_->updateValue(0.0002);
    Clock::stepMs(1100);
    sut_->updateValue(1.0008);
    Clock::stepMs(2200);
    sut_->updateValue(1.0016);
    Clock::stepMs(3300);
    sut_->updateValue(3.0004);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<uint64_t>(statValuesMap.at("Current")),
                testing::Eq(5UL));
}

TEST_F(EnergyStatisticTest, UpdateNanValueExpectMeasurementOff)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);
    Clock::stepMs(2200);
    sut_->updateValue(std::numeric_limits<double>::quiet_NaN());

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<bool>(statValuesMap.at("MeasurementState")),
                testing::Eq(false));
}

TEST_F(EnergyStatisticTest, UpdateOkValueAfterNanExpectMeasurementOn)
{
    sut_->updateValue(10.0);
    Clock::stepMs(1100);
    sut_->updateValue(20.0);
    Clock::stepMs(2200);
    sut_->updateValue(std::numeric_limits<double>::quiet_NaN());
    Clock::stepMs(3300);
    sut_->updateValue(30.0);

    StatValuesMap statValuesMap = sut_->getValuesMap();
    EXPECT_THAT(std::get<bool>(statValuesMap.at("MeasurementState")),
                testing::Eq(true));
}

} // namespace nodemanager