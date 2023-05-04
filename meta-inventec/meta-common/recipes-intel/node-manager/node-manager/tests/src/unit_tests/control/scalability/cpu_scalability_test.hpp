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
#include "control/scalability/cpu_scalability.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "mocks/knob_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class CpuScalabilityTest : public ::testing::Test
{
  public:
    virtual ~CpuScalabilityTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPresence, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&presenceReading_));

        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPackagePowerLimit, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&limitReading_));

        ON_CALL(*devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPackagePower, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(&powerReading_));

        for (int i = 0; i < kMaxCpuNumber; i++)
        {
            ON_CALL(*devManMock_, isKnobSet(KnobType::TurboRatioLimit, i))
                .WillByDefault(testing::Return(false));
            ON_CALL(*devManMock_,
                    registerReadingConsumerHelper(
                        testing::_, ReadingType::cpuAverageFrequency, i))
                .WillByDefault(testing::SaveArg<0>(&frequencyReadings_[i]));
            ON_CALL(
                *devManMock_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPackagePowerCapabilitiesMax, i))
                .WillByDefault(testing::SaveArg<0>(&capabilityReadings_[i]));
        }

        sut_ = std::make_unique<CpuScalability>(devManMock_, true);
    }

    void setupReadings(DeviceIndex cpuCount, double limit, double power,
                       std::vector<double> frequencies,
                       std::vector<double> capabilities)
    {
        propFactor = 1.0 / cpuCount;
        oneWattInFactor = 1.0 / limit;
        std::bitset<8> bs((1 << cpuCount) - 1);
        presenceReading_->updateValue(static_cast<double>(bs.to_ulong()));
        limitReading_->updateValue(limit);
        powerReading_->updateValue(power);
        for (int i = 0; i < kMaxCpuNumber; i++)
        {
            if (bs[i])
            {
                frequencyReadings_[i]->updateValue(frequencies[i]);
                capabilityReadings_[i]->updateValue(capabilities[i]);
            }
            else
            {
                frequencyReadings_[i]->updateValue(
                    std::numeric_limits<double>::quiet_NaN());
                capabilityReadings_[i]->updateValue(
                    std::numeric_limits<double>::quiet_NaN());
            }
        }
    }

    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<ReadingConsumer> presenceReading_;
    std::shared_ptr<ReadingConsumer> limitReading_;
    std::shared_ptr<ReadingConsumer> powerReading_;
    std::vector<std::shared_ptr<ReadingConsumer>> frequencyReadings_{
        kMaxCpuNumber};
    std::vector<std::shared_ptr<ReadingConsumer>> capabilityReadings_{
        kMaxCpuNumber};
    std::unique_ptr<CpuScalability> sut_;
    double propFactor{};
    double oneWattInFactor{};

    void expectFactors(std::vector<double> factors)
    {
        EXPECT_THAT(
            sut_->getFactors(),
            testing::ElementsAre(
                testing::DoubleEq(factors[0]), testing::DoubleEq(factors[1]),
                testing::DoubleEq(factors[2]), testing::DoubleEq(factors[3]),
                testing::DoubleEq(factors[4]), testing::DoubleEq(factors[5]),
                testing::DoubleEq(factors[6]), testing::DoubleEq(factors[7])));
    }
};

TEST_F(CpuScalabilityTest, UnregisterInDescrutcor)
{
    EXPECT_CALL(*devManMock_, unregisterReadingConsumer(
                                  testing::Truly([this](const auto& arg) {
                                      return arg == presenceReading_;
                                  })));
    EXPECT_CALL(*devManMock_,
                unregisterReadingConsumer(testing::Truly(
                    [this](const auto& arg) { return arg == limitReading_; })));
    EXPECT_CALL(*devManMock_,
                unregisterReadingConsumer(testing::Truly(
                    [this](const auto& arg) { return arg == powerReading_; })));
    for (int i = 0; i < kMaxCpuNumber; i++)
    {
        EXPECT_CALL(*devManMock_, unregisterReadingConsumer(testing::Truly(
                                      [this, i](const auto& arg) {
                                          return arg == frequencyReadings_[i];
                                      })));
        EXPECT_CALL(*devManMock_, unregisterReadingConsumer(testing::Truly(
                                      [this, i](const auto& arg) {
                                          return arg == capabilityReadings_[i];
                                      })));
    }
    sut_ = nullptr;
}

TEST_F(CpuScalabilityTest, DefaultsNoUpdateNoCpus)
{
    const auto ret = sut_->getFactors();
    expectFactors({0, 0, 0, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, NaNFrequencyIgnored)
{
    setupReadings(3, 200, 200,
                  {1337.0, 1337.0, std::numeric_limits<double>::quiet_NaN()},
                  {500, 500, 500});
    expectFactors({propFactor, propFactor, propFactor, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, EqualFrequencyEqualFactors)
{
    setupReadings(3, 200, 200, {1337.0, 1337.0, 1337.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor, propFactor, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, LowFrequencyDiffEqualFactors)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1317.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor, propFactor, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, FrequencyDiffDistributeOneWatt)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor - oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, CpuDisappearResetScalability)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor - oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0, 0});
    setupReadings(2, 200, 200, {1367.0, 1397.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor, 0, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, CpuAppearResetScalability)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor - oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0, 0});
    setupReadings(4, 200, 200, {1367.0, 1397.0, 1337.0, 1298.0},
                  {500, 500, 500});
    expectFactors({propFactor, propFactor, propFactor, propFactor, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, FrequencyDiffKeepDistributingOneWatt)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    for (int i = 1; i < 5; i++)
    {
        expectFactors({propFactor, propFactor - (i * oneWattInFactor),
                       propFactor + (i * oneWattInFactor), 0, 0, 0, 0, 0});
    }
}

TEST_F(CpuScalabilityTest, FrequencyDiffDistributeToLowestFrequencyCpu)
{
    setupReadings(4, 200, 200, {1367.0, 5000.0, 1217.0, 100.0},
                  {500, 500, 500, 500});
    expectFactors({0.25, propFactor - oneWattInFactor, propFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0});

    setupReadings(4, 200, 200, {1367.0, 5000.0, 1217.0, 1500.0},
                  {500, 500, 500, 500});
    expectFactors({propFactor, propFactor - 2 * oneWattInFactor,
                   propFactor + oneWattInFactor, propFactor + oneWattInFactor,
                   0, 0, 0, 0});

    setupReadings(4, 200, 200, {1367.0, 5000.0, 1217.0, 1500.0},
                  {500, 500, 500, 500});
    expectFactors({propFactor, propFactor - 3 * oneWattInFactor,
                   propFactor + 2 * oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0});

    setupReadings(4, 200, 200, {1367.0, 5000.0, 1617.0, 1500.0},
                  {500, 500, 500, 500});
    expectFactors({propFactor + oneWattInFactor,
                   propFactor - 4 * oneWattInFactor,
                   propFactor + 2 * oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest,
       FrequencyDiffKeepDistributingOneWattUntilFrequencySimilar)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    for (int i = 1; i < 5; i++)
    {
        expectFactors({propFactor, propFactor - (i * oneWattInFactor),
                       propFactor + (i * oneWattInFactor), 0, 0, 0, 0, 0});
    }
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1317.0}, {500, 500, 500});

    expectFactors({propFactor, propFactor - (4 * oneWattInFactor),
                   propFactor + (4 * oneWattInFactor), 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, TurboRatioSetUseProportionalScalability)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor - oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0, 0});
    EXPECT_CALL(*devManMock_, isKnobSet(KnobType::TurboRatioLimit, 0))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*devManMock_, isKnobSet(KnobType::TurboRatioLimit, 1))
        .WillOnce(testing::Return(true));

    expectFactors({propFactor, propFactor, propFactor, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest, PowerLimitNotAchievedStopDistributing)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    for (int i = 1; i < 5; i++)
    {
        expectFactors({propFactor, propFactor - (i * oneWattInFactor),
                       propFactor + (i * oneWattInFactor), 0, 0, 0, 0, 0});
    }
    setupReadings(3, 200, 150, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor - (4 * oneWattInFactor),
                   propFactor + (4 * oneWattInFactor), 0, 0, 0, 0, 0});

    setupReadings(3, 200, 240, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    expectFactors({propFactor, propFactor - (4 * oneWattInFactor),
                   propFactor + (4 * oneWattInFactor), 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest,
       FrequencyDiffDistributeOneWattUntilMinFactorLimitReached)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0}, {500, 500, 500});
    const auto& factorLimit = 0.75 * propFactor;
    int iOverLimit = 0;
    // iterate until 0.25 factor distributed + 4 iterations
    for (int i = 1; i < ((0.25 / 3) / oneWattInFactor) + 4; i++)
    {
        if (propFactor - (i * oneWattInFactor) < factorLimit)
        {
            // if 0.25 factor distributed, distribution should stop
            iOverLimit++;
            expectFactors({propFactor,
                           propFactor - ((i - iOverLimit) * oneWattInFactor),
                           propFactor + ((i - iOverLimit) * oneWattInFactor), 0,
                           0, 0, 0, 0});
        }
        else
        {
            expectFactors({propFactor, propFactor - (i * oneWattInFactor),
                           propFactor + (i * oneWattInFactor), 0, 0, 0, 0, 0});
        }
    }
}

TEST_F(CpuScalabilityTest, NaNCapabilityIgnored)
{
    setupReadings(3, 200, 200, {1367.0, 1397.0, 1217.0},
                  {500, std::numeric_limits<double>::quiet_NaN(), 500});
    expectFactors({propFactor, propFactor - oneWattInFactor,
                   propFactor + oneWattInFactor, 0, 0, 0, 0, 0});
}

TEST_F(CpuScalabilityTest,
       FrequencyDiffDistributeOneWattUntilMaxFactorLimitReached)
{
    setupReadings(3, 300, 300, {1367.0, 1397.0, 1217.0}, {103, 103, 103});

    for (int i = 1; i < 3; i++)
    {
        expectFactors({propFactor, propFactor - (i * oneWattInFactor),
                       propFactor + (i * oneWattInFactor), 0, 0, 0, 0, 0});
    }
    expectFactors({propFactor, propFactor - (2 * oneWattInFactor),
                   propFactor + (2 * oneWattInFactor), 0, 0, 0, 0, 0});
}