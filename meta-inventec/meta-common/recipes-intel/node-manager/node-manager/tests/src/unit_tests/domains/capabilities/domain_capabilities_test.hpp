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

#include "common_types.hpp"
#include "config/config.hpp"
#include "domains/capabilities/component_capabilities.hpp"
#include "domains/capabilities/domain_capabilities.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "utils/domain_params.hpp"

#include <variant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class DomainCapabilitiesTest : public ::testing::Test
{
  protected:
    DomainCapabilitiesTest()
    {
    }

    virtual void SetUp() override
    {
        ReadingType minCapReadingType =
            ReadingType::cpuPackagePowerCapabilitiesMin;
        ReadingType maxCapReadingType =
            ReadingType::cpuPackagePowerCapabilitiesMax;

        ON_CALL(*devManMock_, registerReadingConsumerHelper(
                                  testing::_, minCapReadingType, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&rcMin_));
        ON_CALL(*devManMock_, registerReadingConsumerHelper(
                                  testing::_, maxCapReadingType, kAllDevices))
            .WillByDefault(testing::DoAll(testing::SaveArg<0>(&rcMax_),
                                          testing::SaveArg<0>(&rcMaxRated_)));

        clearConfig();

        sut_ = std::make_unique<DomainCapabilities>(
            minCapReadingType, maxCapReadingType, devManMock_, 1000,
            [this]() { return capabilitiesChangeCallbackMock.Call(); },
            DomainId::AcTotalPower);

        const double minCap = 10.0;
        const double maxCap = 150.0;
        const double maxRatedCap = 200.0;

        rcMin_->updateValue(minCap);
        rcMax_->updateValue(maxCap);
        rcMaxRated_->updateValue(maxRatedCap);
    }

    virtual ~DomainCapabilitiesTest() = default;

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> rcMin_;
    std::shared_ptr<ReadingConsumer> rcMax_;
    std::shared_ptr<ReadingConsumer> rcMaxRated_;
    std::unique_ptr<DomainCapabilities> sut_;
    testing::MockFunction<void(void)> capabilitiesChangeCallbackMock;

    void clearConfig()
    {
        PowerRange config = {0};
        Config::getInstance().update(config);
    }
};

TEST_F(DomainCapabilitiesTest, UpdateMinReadingExpectGetMinCorrectValue)
{
    const double minCap = 20.0;
    this->rcMin_->updateValue(minCap);

    EXPECT_DOUBLE_EQ(this->sut_->getMin(), minCap);
}

TEST_F(DomainCapabilitiesTest, UpdateMaxReadingExpectGetMaxCorrectValue)
{
    const double maxCap = 300.0;
    this->rcMax_->updateValue(maxCap);

    EXPECT_DOUBLE_EQ(this->sut_->getMax(), maxCap);
}

TEST_F(DomainCapabilitiesTest,
       UpdateBelowMaxRatedReadingExpectGetMaxRatedCorrectValue)
{
    const double maxRatedCap = 180.0;
    this->rcMaxRated_->updateValue(maxRatedCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMaxRated(), maxRatedCap);
}

TEST_F(DomainCapabilitiesTest,
       UpdateOverMaxRatedReadingExpectGetMaxRatedCorrectValue)
{
    const double maxRatedCap = 220.0;
    this->rcMaxRated_->updateValue(maxRatedCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMaxRated(), maxRatedCap);
}

TEST_F(DomainCapabilitiesTest, GetNameExpectDomainName)
{
    EXPECT_EQ(this->sut_->getName(), "Domain");
}

TEST_F(DomainCapabilitiesTest, GetValuesMapExpectCorrectValues)
{
    const double minCap = 15.0;
    this->rcMin_->updateValue(minCap);
    const double maxCap = 420.0;
    this->rcMax_->updateValue(maxCap);

    CapabilitiesValuesMap capabilitiesMap = {{"Min", minCap}, {"Max", maxCap}};

    EXPECT_EQ(this->sut_->getValuesMap(), capabilitiesMap);
}

TEST_F(DomainCapabilitiesTest, GetMaxCorrectionTimeInMsExpectCorrectValue)
{
    EXPECT_EQ(this->sut_->getMaxCorrectionTimeInMs(),
              static_cast<uint32_t>(60000));
}

TEST_F(DomainCapabilitiesTest, GetMinCorrectionTimeInMsExpectCorrectValue)
{
    EXPECT_EQ(this->sut_->getMinCorrectionTimeInMs(),
              static_cast<uint32_t>(1000));
}

TEST_F(DomainCapabilitiesTest, GetMaxStatReportingPeriodExpectCorrectValue)
{
    EXPECT_EQ(this->sut_->getMaxStatReportingPeriod(),
              static_cast<uint16_t>(3600));
}

TEST_F(DomainCapabilitiesTest, GetMinStatReportingPeriodExpectCorrectValue)
{
    EXPECT_EQ(this->sut_->getMinStatReportingPeriod(),
              static_cast<uint16_t>(1));
}

TEST_F(DomainCapabilitiesTest, SetMinExpectValueLocked)
{
    const double minCap = 37.0;
    const double newMinCap = 16.0;

    this->sut_->setMin(minCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), minCap);

    this->rcMin_->updateValue(newMinCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), minCap);
}

TEST_F(DomainCapabilitiesTest, SetMaxExpectValueLocked)
{
    const double maxCap = 480.0;
    const double newMaxCap = 354.0;

    this->sut_->setMax(maxCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), maxCap);

    this->rcMax_->updateValue(newMaxCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), maxCap);
}

TEST_F(DomainCapabilitiesTest, UnlockMinExpectValueUpdated)
{
    const double minCap = 14.0;
    const double newMinCap = 16.0;

    this->sut_->setMin(minCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), minCap);

    this->sut_->setMin(0.0);

    this->rcMin_->updateValue(newMinCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), newMinCap);
}

TEST_F(DomainCapabilitiesTest, UnlockMaxExpectValueUpdated)
{
    const double maxCap = 377.0;
    const double newMaxCap = 354.0;

    this->sut_->setMax(maxCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), maxCap);

    this->sut_->setMax(0.0);

    this->rcMax_->updateValue(newMaxCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), newMaxCap);
}

TEST_F(DomainCapabilitiesTest, ChangeMinCapabilityExpectCallbackCalled)
{
    const double minCap = 20.0;
    EXPECT_CALL(capabilitiesChangeCallbackMock, Call());
    this->rcMin_->updateValue(minCap);
}

TEST_F(DomainCapabilitiesTest, ChangeMaxCapabilityExpectCallbackCalled)
{
    const double maxCap = 420.0;
    EXPECT_CALL(capabilitiesChangeCallbackMock, Call());
    this->sut_->setMax(maxCap);
}

TEST_F(DomainCapabilitiesTest, SetMinCapabilityTwiceExpectCallbackCalledOnce)
{
    const double minCap = 20.0;
    EXPECT_CALL(capabilitiesChangeCallbackMock, Call());
    this->sut_->setMin(minCap);
    this->sut_->setMin(minCap);
}

TEST_F(DomainCapabilitiesTest, SetMaxCapabilityTwiceExpectCallbackCalledOnce)
{
    const double maxCap = 420.0;
    EXPECT_CALL(capabilitiesChangeCallbackMock, Call());
    this->sut_->setMax(maxCap);
    this->sut_->setMax(maxCap);
}

class DomainCapabilitiesNulloptTest : public ::testing::Test
{
  protected:
    DomainCapabilitiesNulloptTest() = default;

    virtual void SetUp() override
    {
        clearConfig();
        sut_ = std::make_unique<DomainCapabilities>(std::nullopt, std::nullopt,
                                                    devManMock_, 1000, nullptr,
                                                    DomainId::AcTotalPower);
    }

    virtual ~DomainCapabilitiesNulloptTest() = default;

    void clearConfig()
    {
        PowerRange config = {0};
        Config::getInstance().update(config);
    }

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::unique_ptr<DomainCapabilities> sut_;
};

TEST_F(DomainCapabilitiesNulloptTest, GetMinCorrectValue)
{
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), 0.0);
}

TEST_F(DomainCapabilitiesNulloptTest, GetMaxCorrectValue)
{
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), 32767.0);
}

TEST_F(DomainCapabilitiesNulloptTest, GetValuesMapExpectCorrectValues)
{
    CapabilitiesValuesMap capabilitiesMap = {{"Min", 0.0}, {"Max", 32767.0}};

    EXPECT_EQ(this->sut_->getValuesMap(), capabilitiesMap);
}

TEST_F(DomainCapabilitiesNulloptTest, SetMinExpectValueLocked)
{
    const double newMinCap = 16.0;

    this->sut_->setMin(newMinCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), newMinCap);
}

TEST_F(DomainCapabilitiesNulloptTest, SetMaxExpectValueLocked)
{
    const double newMaxCap = 354.0;

    this->sut_->setMax(newMaxCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), newMaxCap);
}

TEST_F(DomainCapabilitiesNulloptTest, UnlockMinExpectValueRestored)
{
    const double newMinCap = 16.0;

    this->sut_->setMin(newMinCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), newMinCap);

    this->sut_->setMin(0.0);
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), 0.0);
}

TEST_F(DomainCapabilitiesNulloptTest, UnlockMaxExpectValueRestored)
{
    const double newMaxCap = 354.2;

    this->sut_->setMax(newMaxCap);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), newMaxCap);

    this->sut_->setMax(0.0);
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), 0.0);
}
