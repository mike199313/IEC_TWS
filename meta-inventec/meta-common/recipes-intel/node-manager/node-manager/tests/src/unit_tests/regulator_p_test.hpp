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

#pragma once
#include "mocks/devices_manager_mock.hpp"
#include "regulator_p.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static const uint32_t kGain = 1.23;

class RegulatorPTest : public ::testing::Test
{
  public:
    RegulatorPTest()
    {
    }

    virtual void SetUp() override
    {
        EXPECT_CALL(*devicesManager_,
                    registerReadingConsumerHelper(
                        testing::_, ReadingType::dcPlatformPower, kAllDevices))
            .WillOnce(testing::SaveArg<0>(&readSampleHandler));

        EXPECT_CALL(*devicesManager_, unregisterReadingConsumer(testing::Truly(
                                          [this](const auto& arg) {
                                              return arg == readSampleHandler;
                                          })));

        sut_ = std::make_unique<RegulatorP>(devicesManager_, kGain,
                                            ReadingType::dcPlatformPower);
    }

  protected:
    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devicesManager_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();
    std::unique_ptr<RegulatorIf> sut_;
    std::shared_ptr<ReadingConsumer> readSampleHandler;
};

TEST_F(RegulatorPTest, CalculateControlSignal)
{
    double setPoint{4.56};

    readSampleHandler->updateValue(7.89);

    EXPECT_DOUBLE_EQ(sut_->calculateControlSignal(setPoint),
                     kGain * (setPoint - 7.89));
}

TEST_F(RegulatorPTest, InputSignalNotAvailable_NanControlSignalExpected)
{
    double setPoint{std::numeric_limits<double>::quiet_NaN()};
    readSampleHandler->updateValue(1.0);

    EXPECT_TRUE(std::isnan(sut_->calculateControlSignal(setPoint)));
}

TEST_F(RegulatorPTest, OutputSignalNotAvailable_NanControlSignalExpected)
{
    double setPoint{1.0};
    readSampleHandler->updateValue(std::numeric_limits<double>::quiet_NaN());

    EXPECT_TRUE(std::isnan(sut_->calculateControlSignal(setPoint)));
}

TEST_F(RegulatorPTest, OnStart_NanControlSignalExpected)
{
    double setPoint{1.0};
    EXPECT_TRUE(std::isnan(sut_->calculateControlSignal(setPoint)));
}