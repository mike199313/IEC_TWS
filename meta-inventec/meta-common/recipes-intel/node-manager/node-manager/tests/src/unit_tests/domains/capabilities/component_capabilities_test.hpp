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
#include "domains/capabilities/component_capabilities.hpp"
#include "domains/capabilities/domain_capabilities.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "utils/domain_params.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class ComponentCapabilitiesTest : public ::testing::Test
{
  protected:
    ComponentCapabilitiesTest() = default;

    virtual void SetUp() override
    {
        ReadingType minCapReadingType =
            ReadingType::cpuPackagePowerCapabilitiesMin;
        ReadingType maxCapReadingType =
            ReadingType::cpuPackagePowerCapabilitiesMax;
        DeviceIndex deviceIdx = 3;

        ON_CALL(*devManMock_, registerReadingConsumerHelper(
                                  testing::_, minCapReadingType, deviceIdx))
            .WillByDefault(testing::SaveArg<0>(&rcMin_));
        ON_CALL(*devManMock_, registerReadingConsumerHelper(
                                  testing::_, maxCapReadingType, deviceIdx))
            .WillByDefault(testing::SaveArg<0>(&rcMax_));

        sut_ = std::make_unique<ComponentCapabilities>(
            minCapReadingType, maxCapReadingType, deviceIdx, devManMock_);

        const double minCap = 10.0;
        const double maxCap = 150.0;

        rcMin_->updateValue(minCap);
        rcMax_->updateValue(maxCap);
    }

    virtual ~ComponentCapabilitiesTest() = default;

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> rcMin_;
    std::shared_ptr<ReadingConsumer> rcMax_;
    std::unique_ptr<ComponentCapabilities> sut_;
};

TEST_F(ComponentCapabilitiesTest, UpdateMinReadingExpectGetMinCorrectValue)
{
    const double minCap = 20.0;
    this->rcMin_->updateValue(minCap);

    EXPECT_DOUBLE_EQ(this->sut_->getMin(), minCap);
}

TEST_F(ComponentCapabilitiesTest, UpdateMaxReadingExpectGetMaxCorrectValue)
{
    const double maxCap = 300.0;
    this->rcMax_->updateValue(maxCap);

    EXPECT_DOUBLE_EQ(this->sut_->getMax(), maxCap);
}

TEST_F(ComponentCapabilitiesTest, GetNameExpectComponentName)
{
    EXPECT_EQ(this->sut_->getName(), "Component_3");
}

TEST_F(ComponentCapabilitiesTest, GetValuesMapExpectCorrectValues)
{
    const double minCap = 15.0;
    this->rcMin_->updateValue(minCap);
    const double maxCap = 420.0;
    this->rcMax_->updateValue(maxCap);

    CapabilitiesValuesMap capabilitiesMap = {{"Min", minCap}, {"Max", maxCap}};

    EXPECT_EQ(this->sut_->getValuesMap(), capabilitiesMap);
}

class ComponentCapabilitiesNulloptTest : public ::testing::Test
{
  protected:
    ComponentCapabilitiesNulloptTest() = default;

    virtual void SetUp() override
    {
        DeviceIndex deviceIdx = 4;

        sut_ = std::make_unique<ComponentCapabilities>(
            std::nullopt, std::nullopt, deviceIdx, devManMock_);
    }

    virtual ~ComponentCapabilitiesNulloptTest() = default;

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::unique_ptr<ComponentCapabilities> sut_;
};

TEST_F(ComponentCapabilitiesNulloptTest, GetMinCorrectValue)
{
    EXPECT_DOUBLE_EQ(this->sut_->getMin(), 0.0);
}

TEST_F(ComponentCapabilitiesNulloptTest, GetMaxCorrectValue)
{
    EXPECT_DOUBLE_EQ(this->sut_->getMax(), 32767.0);
}

TEST_F(ComponentCapabilitiesNulloptTest, GetValuesMapExpectCorrectValues)
{
    CapabilitiesValuesMap capabilitiesMap = {{"Min", 0.0}, {"Max", 32767.0}};

    EXPECT_EQ(this->sut_->getValuesMap(), capabilitiesMap);
}