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

#include "budgeting/budgeting.hpp"
#include "mocks/compound_domain_budgeting_mock.hpp"
#include "mocks/control_mock.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class BudgetingTestBase
{
  protected:
    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();
    std::unique_ptr<::testing::NiceMock<CompoundDomainBudgetingMock>>
        compoundBudgetingMock_ = std::make_unique<
            ::testing::NiceMock<CompoundDomainBudgetingMock>>();
    std::shared_ptr<::testing::NiceMock<ControlMock>> controlMock_ =
        std::make_shared<::testing::NiceMock<ControlMock>>();
    std::unique_ptr<Budgeting> budgeting_ = std::make_unique<Budgeting>(
        devManMock_, std::move(compoundBudgetingMock_), controlMock_);
};

class BudgetingTest : public ::testing::Test
{
  protected:
    BudgetingTest() = default;
    virtual ~BudgetingTest() = default;

    virtual void SetUp() override
    {
        EXPECT_CALL(
            *devManMock_,
            registerReadingConsumerHelper(
                testing::_, ReadingType::platformPowerEfficiency, kAllDevices))
            .WillOnce(testing::SaveArg<0>(&rc_));

        EXPECT_CALL(*devManMock_,
                    unregisterReadingConsumer(testing::Truly(
                        [this](const auto& arg) { return arg == rc_; })));

        EXPECT_CALL(*controlMock_,
                    setBudget(testing::_, std::optional<Limit>(std::nullopt)))
            .WillRepeatedly(testing::Return());

        budgeting_ = std::make_unique<Budgeting>(
            devManMock_, std::move(compoundBudgetingMock_), controlMock_);
    };
    std::shared_ptr<ReadingConsumer> rc_;
    std::unique_ptr<Budgeting> budgeting_;
    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();
    std::unique_ptr<::testing::NiceMock<CompoundDomainBudgetingMock>>
        compoundBudgetingMock_ = std::make_unique<
            ::testing::NiceMock<CompoundDomainBudgetingMock>>();
    std::shared_ptr<::testing::NiceMock<ControlMock>> controlMock_ =
        std::make_shared<::testing::NiceMock<ControlMock>>();
};

TEST_F(BudgetingTest, SetsEmptyBudgetWhenNoLimitSet)
{
    EXPECT_CALL(*controlMock_, setBudget(testing::_, testing::_)).Times(0);
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::cpuSubsystem,
                                         std::optional<Limit>(std::nullopt)));
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::dcTotalPower,
                                         std::optional<Limit>(std::nullopt)));
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::memorySubsystem,
                                         std::optional<Limit>(std::nullopt)));
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::pcie,
                                         std::optional<Limit>(std::nullopt)));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainLimit_CpuNonAggressive)
{
    double limitValue = 100.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::CpuSubsystem, kComponentIdAll, limitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::cpuSubsystem,
                          std::optional<Limit>(Limit{limitValue, strategy})));
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::dcTotalPower,
                                         std::optional<Limit>(std::nullopt)));
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::memorySubsystem,
                                         std::optional<Limit>(std::nullopt)));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainLimit_MemoryNonAggressive)
{
    double limitValue = 10.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::MemorySubsystem, kComponentIdAll, limitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::memorySubsystem,
                          std::optional<Limit>(Limit{limitValue, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainLimit_DcPlatformNonAggressive)
{
    double limitValue = 150.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::DcTotalPower, kComponentIdAll, limitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{limitValue, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainLimit_AcPlatformNonAggressiveEfficiency100)
{
    double limitValue = 200.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;
    rc_->updateValue(static_cast<double>(1.0));

    budgeting_->setLimit(DomainId::AcTotalPower, kComponentIdAll, limitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{limitValue, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainLimit_AcPlatformNonAggressiveEfficiency75)
{
    double limitValue = 200.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;
    rc_->updateValue(0.75);

    budgeting_->setLimit(DomainId::AcTotalPower, kComponentIdAll, limitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{150.0, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainLimit_AcPlatformNonAggressiveEfficiencyNanValue)
{
    double limitValue = 250.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;
    rc_->updateValue(std::numeric_limits<double>::quiet_NaN());

    budgeting_->setLimit(DomainId::AcTotalPower, kComponentIdAll, limitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{250.0, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest,
       SetDomainsLimit_AcPlatformNonAggressiveDcPlatformNonAggressive_AcWins)
{
    double acLimitValue = 100.0;
    double dcLimitValue = 120.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::AcTotalPower, kComponentIdAll, acLimitValue,
                         strategy);
    budgeting_->setLimit(DomainId::DcTotalPower, kComponentIdAll, dcLimitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{acLimitValue, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest,
       SetDomainsLimit_AcPlatformNonAggressiveDcPlatformNonAggressive_DcWins)
{
    double acLimitValue = 140.0;
    double dcLimitValue = 120.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::AcTotalPower, kComponentIdAll, acLimitValue,
                         strategy);
    budgeting_->setLimit(DomainId::DcTotalPower, kComponentIdAll, dcLimitValue,
                         strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{dcLimitValue, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetDomainsLimit_MultiplePtamDomains)
{
    double dcLimitValue = 80.0;
    double cpuLimitValue = 50.0;
    double memoryLimitValue = 20.0;
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::DcTotalPower, kComponentIdAll, dcLimitValue,
                         strategy);
    budgeting_->setLimit(DomainId::CpuSubsystem, kComponentIdAll, cpuLimitValue,
                         strategy);
    budgeting_->setLimit(DomainId::MemorySubsystem, kComponentIdAll,
                         memoryLimitValue, strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::dcTotalPower,
                          std::optional<Limit>(Limit{dcLimitValue, strategy})));
    EXPECT_CALL(*controlMock_, setBudget(RaplDomainId::cpuSubsystem,
                                         std::optional<Limit>(
                                             Limit{cpuLimitValue, strategy})));
    EXPECT_CALL(
        *controlMock_,
        setBudget(RaplDomainId::memorySubsystem,
                  std::optional<Limit>(Limit{memoryLimitValue, strategy})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest,
       SetDomainsLimit_DifferentStartegiesDifferentValues_AggressiveWins)
{
    double lowestLimitValue = 10.0;
    double mediumLimitValue = 20.0;
    double highestLimitValue = 30.0;
    DomainId domainId = DomainId::CpuSubsystem;

    budgeting_->setLimit(domainId, kComponentIdAll, lowestLimitValue,
                         BudgetingStrategy::aggressive);
    budgeting_->setLimit(domainId, kComponentIdAll, mediumLimitValue,
                         BudgetingStrategy::nonAggressive);
    budgeting_->setLimit(domainId, kComponentIdAll, highestLimitValue,
                         BudgetingStrategy::immediate);

    EXPECT_CALL(
        *controlMock_,
        setBudget(RaplDomainId::cpuSubsystem,
                  std::optional<Limit>(
                      Limit{lowestLimitValue, BudgetingStrategy::aggressive})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest,
       SetDomainsLimit_DifferentStartegiesDifferentValues_NonAggressiveWins)
{
    double lowestLimitValue = 10.0;
    double mediumLimitValue = 20.0;
    double highestLimitValue = 30.0;
    DomainId domainId = DomainId::CpuSubsystem;

    budgeting_->setLimit(domainId, kComponentIdAll, mediumLimitValue,
                         BudgetingStrategy::aggressive);
    budgeting_->setLimit(domainId, kComponentIdAll, lowestLimitValue,
                         BudgetingStrategy::nonAggressive);
    budgeting_->setLimit(domainId, kComponentIdAll, highestLimitValue,
                         BudgetingStrategy::immediate);

    EXPECT_CALL(
        *controlMock_,
        setBudget(RaplDomainId::cpuSubsystem,
                  std::optional<Limit>(Limit{
                      lowestLimitValue, BudgetingStrategy::nonAggressive})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest,
       SetDomainsLimit_DifferentStartegiesDifferentValues_ImmediateWins)
{
    double lowestLimitValue = 10.0;
    double mediumLimitValue = 20.0;
    double highestLimitValue = 30.0;
    DomainId domainId = DomainId::CpuSubsystem;

    budgeting_->setLimit(domainId, kComponentIdAll, mediumLimitValue,
                         BudgetingStrategy::aggressive);
    budgeting_->setLimit(domainId, kComponentIdAll, highestLimitValue,
                         BudgetingStrategy::nonAggressive);
    budgeting_->setLimit(domainId, kComponentIdAll, lowestLimitValue,
                         BudgetingStrategy::immediate);

    EXPECT_CALL(
        *controlMock_,
        setBudget(RaplDomainId::cpuSubsystem,
                  std::optional<Limit>(
                      Limit{lowestLimitValue, BudgetingStrategy::immediate})));

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetComponentsLimit_Cpu)
{
    std::vector<double> limitValues = {11.0, 22.0, 33.0, 44.0,
                                       55.0, 66.0, 77.0, 88.0};
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    for (DeviceIndex index = 0; index < kMaxCpuNumber; ++index)
    {
        EXPECT_CALL(*controlMock_,
                    setComponentBudget(RaplDomainId::cpuSubsystem, index,
                                       std::optional<Limit>(Limit{
                                           limitValues[index], strategy})));
    }

    for (DeviceIndex index = 0; index < kMaxCpuNumber; ++index)
    {
        ASSERT_NO_THROW(budgeting_->setLimit(DomainId::CpuSubsystem, index,
                                             limitValues[index], strategy));
    }

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetComponentsLimit_Memory)
{
    std::vector<double> limitValues = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    for (DeviceIndex index = 0; index < kMaxCpuNumber; ++index)
    {
        EXPECT_CALL(*controlMock_,
                    setComponentBudget(RaplDomainId::memorySubsystem, index,
                                       std::optional<Limit>(Limit{
                                           limitValues[index], strategy})));
    }

    for (DeviceIndex index = 0; index < kMaxCpuNumber; ++index)
    {
        ASSERT_NO_THROW(budgeting_->setLimit(DomainId::MemorySubsystem, index,
                                             limitValues[index], strategy));
    }

    ASSERT_NO_THROW(budgeting_->run());
}

TEST_F(BudgetingTest, SetsLimitsForPcieDomain)
{
    BudgetingStrategy strategy = BudgetingStrategy::nonAggressive;

    budgeting_->setLimit(DomainId::Pcie, kComponentIdAll, 5.0, strategy);

    EXPECT_CALL(*controlMock_,
                setBudget(RaplDomainId::pcie,
                          std::optional<Limit>(Limit{5.0, strategy})));

    budgeting_->run();
}

struct BudgetingSetLimitParam
{
    double limit;
    BudgetingStrategy startegy;
};

struct BudgetingIsActiveParam
{
    bool retValue;
};

struct ControlIsDomainLimitActiveParam
{
    RaplDomainId domainId;
    bool retValue;
};

struct SingleLimitParamBundle
{
    BudgetingSetLimitParam setLimit;
    BudgetingIsActiveParam isActiveAggressive;
    BudgetingIsActiveParam isActiveNonAggressive;
    BudgetingIsActiveParam isActiveImmediate;
    ControlIsDomainLimitActiveParam isDomainLimitActive;
};

class BudgetingIsActiveSetSingleLimitTest
    : public BudgetingTestBase,
      public ::testing::TestWithParam<SingleLimitParamBundle>
{
  protected:
    SingleLimitParamBundle params = GetParam();
};

INSTANTIATE_TEST_SUITE_P(
    SingleLimit, BudgetingIsActiveSetSingleLimitTest,
    ::testing::Values(
        // SetAggressiveLimitExpectAggressiveLimitIsActive
        SingleLimitParamBundle{{1.0, BudgetingStrategy::aggressive},
                               {true},
                               {false},
                               {false},
                               {RaplDomainId::cpuSubsystem, true}},
        // SetNonAggressiveLimitExpectNonAggressiveLimitIsActive
        SingleLimitParamBundle{{1.0, BudgetingStrategy::nonAggressive},
                               {false},
                               {true},
                               {false},
                               {RaplDomainId::cpuSubsystem, true}},
        // SetImmediateLimitExpectImmediateLimitIsActive
        SingleLimitParamBundle{{1.0, BudgetingStrategy::immediate},
                               {false},
                               {false},
                               {true},
                               {RaplDomainId::cpuSubsystem, true}},
        // ControlDomainIsNotActiveExpectLimitIsNotActive
        SingleLimitParamBundle{{1.0, BudgetingStrategy::immediate},
                               {false},
                               {false},
                               {false},
                               {RaplDomainId::cpuSubsystem, false}}));

TEST_P(BudgetingIsActiveSetSingleLimitTest, SingleLimit)
{
    budgeting_->setLimit(DomainId::CpuSubsystem, kComponentIdAll,
                         params.setLimit.limit, params.setLimit.startegy);

    ASSERT_NO_THROW(budgeting_->run());

    EXPECT_CALL(*controlMock_, isDomainLimitActive(RaplDomainId::cpuSubsystem))
        .WillOnce(testing::Return(params.isDomainLimitActive.retValue));

    EXPECT_EQ(budgeting_->isActive(DomainId::CpuSubsystem, kComponentIdAll,
                                   BudgetingStrategy::aggressive),
              params.isActiveAggressive.retValue);
    EXPECT_EQ(budgeting_->isActive(DomainId::CpuSubsystem, kComponentIdAll,
                                   BudgetingStrategy::nonAggressive),
              params.isActiveNonAggressive.retValue);
    EXPECT_EQ(budgeting_->isActive(DomainId::CpuSubsystem, kComponentIdAll,
                                   BudgetingStrategy::immediate),
              params.isActiveImmediate.retValue);
}

struct MultipleLimitParamBundle
{
    BudgetingSetLimitParam setLimitAggressive;
    BudgetingSetLimitParam setLimitNonAggressive;
    BudgetingSetLimitParam setLimitImmediate;
    BudgetingIsActiveParam isActiveAggressive;
    BudgetingIsActiveParam isActiveNonAggressive;
    BudgetingIsActiveParam isActiveImmediate;
    ControlIsDomainLimitActiveParam isDomainLimitActive;
};

class BudgetingIsActiveSetMultipleLimitTest
    : public BudgetingTestBase,
      public ::testing::TestWithParam<MultipleLimitParamBundle>
{
  protected:
    MultipleLimitParamBundle params = GetParam();
};

INSTANTIATE_TEST_SUITE_P(
    MultipleLimit, BudgetingIsActiveSetMultipleLimitTest,
    ::testing::Values(
        // SetAggressiveLimitMostRestrictiveExpectAggressiveLimitIsActive
        MultipleLimitParamBundle{{1.0, BudgetingStrategy::aggressive},
                                 {10.0, BudgetingStrategy::nonAggressive},
                                 {100.0, BudgetingStrategy::immediate},
                                 {true},
                                 {false},
                                 {false},
                                 {RaplDomainId::cpuSubsystem, true}},
        // SetNonAggressiveLimitMostRestrictiveExpectNonAggressiveLimitIsActive
        MultipleLimitParamBundle{{10.0, BudgetingStrategy::aggressive},
                                 {1.0, BudgetingStrategy::nonAggressive},
                                 {100.0, BudgetingStrategy::immediate},
                                 {false},
                                 {true},
                                 {false},
                                 {RaplDomainId::cpuSubsystem, true}},
        // SetImmediateLimitMostRestrictiveExpectImmediateLimitIsActive
        MultipleLimitParamBundle{{100.0, BudgetingStrategy::aggressive},
                                 {10.0, BudgetingStrategy::nonAggressive},
                                 {1.0, BudgetingStrategy::immediate},
                                 {false},
                                 {false},
                                 {true},
                                 {RaplDomainId::cpuSubsystem, true}},
        // SetTheSameLimitForAllStartegiesExpectAggressiveLimitIsActive
        MultipleLimitParamBundle{{100.0, BudgetingStrategy::aggressive},
                                 {100.0, BudgetingStrategy::nonAggressive},
                                 {100.0, BudgetingStrategy::immediate},
                                 {true},
                                 {false},
                                 {false},
                                 {RaplDomainId::cpuSubsystem, true}}));

TEST_P(BudgetingIsActiveSetMultipleLimitTest, MultipleLimit)
{
    budgeting_->setLimit(DomainId::CpuSubsystem, kComponentIdAll,
                         params.setLimitAggressive.limit,
                         params.setLimitAggressive.startegy);
    budgeting_->setLimit(DomainId::CpuSubsystem, kComponentIdAll,
                         params.setLimitNonAggressive.limit,
                         params.setLimitNonAggressive.startegy);
    budgeting_->setLimit(DomainId::CpuSubsystem, kComponentIdAll,
                         params.setLimitImmediate.limit,
                         params.setLimitImmediate.startegy);

    ASSERT_NO_THROW(budgeting_->run());

    EXPECT_CALL(*controlMock_, isDomainLimitActive(RaplDomainId::cpuSubsystem))
        .WillRepeatedly(testing::Return(params.isDomainLimitActive.retValue));

    EXPECT_EQ(budgeting_->isActive(DomainId::CpuSubsystem, kComponentIdAll,
                                   BudgetingStrategy::aggressive),
              params.isActiveAggressive.retValue);
    EXPECT_EQ(budgeting_->isActive(DomainId::CpuSubsystem, kComponentIdAll,
                                   BudgetingStrategy::nonAggressive),
              params.isActiveNonAggressive.retValue);
    EXPECT_EQ(budgeting_->isActive(DomainId::CpuSubsystem, kComponentIdAll,
                                   BudgetingStrategy::immediate),
              params.isActiveImmediate.retValue);
}
