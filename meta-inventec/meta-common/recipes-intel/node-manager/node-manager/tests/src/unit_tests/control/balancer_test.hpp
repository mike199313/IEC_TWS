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
#include "control/balancer.hpp"
#include "mocks/devices_manager_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr double kUnassigned{-1.0};

class BalancerTest : public ::testing::Test
{
  public:
    BalancerTest()
    {
    }

  protected:
    std::shared_ptr<::testing::NiceMock<DevicesManagerMock>> devManMock_ =
        std::make_shared<::testing::NiceMock<DevicesManagerMock>>();

    class ProportionalScalabilityFactor : public ScalabilityIf
    {
      public:
        ProportionalScalabilityFactor(const std::vector<bool> cpuMap) :
            cpuMap_(cpuMap), activeCpus_(0)
        {
            for (const auto&& cpu : cpuMap_)
            {
                activeCpus_ += cpu ? 1 : 0;
            }
        }
        virtual ~ProportionalScalabilityFactor() = default;
        std::vector<double> getFactors() override
        {
            std::vector<double> ret(cpuMap_.size(), 0.0);
            double factor = 1.0 / activeCpus_;
            for (size_t i = 0; i < cpuMap_.size(); i++)
            {
                ret[i] = cpuMap_[i] ? factor : 0.0;
            }
            return ret;
        }

      private:
        std::vector<bool> cpuMap_;
        size_t activeCpus_;
    };

    std::unique_ptr<Balancer>
        prepareBalancerForNCpus(const std::vector<bool> cpuMap)
    {
        scalFac_ = std::make_shared<ProportionalScalabilityFactor>(cpuMap);
        return std::make_unique<Balancer>(devManMock_,
                                          KnobType::CpuPackagePower, scalFac_);
    }
    std::vector<bool> cpuMap_ = std::vector<bool>(kMaxCpuNumber, true);
    std::shared_ptr<ProportionalScalabilityFactor> scalFac_ =
        std::make_shared<ProportionalScalabilityFactor>(cpuMap_);
    std::unique_ptr<Balancer> sut_ = std::make_unique<Balancer>(
        devManMock_, KnobType::CpuPackagePower, scalFac_);
};

TEST_F(BalancerTest, SetComponentLimitForDeviceIdOutOfRangeThrowsAlways)
{
    Limit limit = {200.0, BudgetingStrategy::nonAggressive};
    ASSERT_ANY_THROW(sut_->setComponentLimit(8, limit));
}

// TODO: parametrize below tests so that they use various (N) CPUs
//       numbers and expect different budget distribution
TEST_F(BalancerTest, DistributeBudgetWithNoComponentLimitForNCpus)
{
    const std::vector<bool> cpuMap = {true,  true,  true,  false,
                                      false, false, false, false};
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(Limit{300.0, BudgetingStrategy::nonAggressive});

    std::vector<double> expectedHwLimits{100.0,       100.0,       100.0,
                                         kUnassigned, kUnassigned, kUnassigned,
                                         kUnassigned, kUnassigned};
    for (unsigned deviceindex = 0; deviceindex < cpuMap.size(); deviceindex++)
    {
        if (cpuMap[deviceindex])
        {
            EXPECT_CALL(
                *devManMock_,
                setKnobValue(testing::_, deviceindex,
                             testing::DoubleEq(expectedHwLimits[deviceindex])));
        }
        else
        {
            EXPECT_CALL(*devManMock_,
                        setKnobValue(testing::_, deviceindex, testing::_))
                .Times(0);
        }
    }
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DistributeBudgetWithNoComponentLimitForNoCpus)
{
    const std::vector<bool> emptyCpuMap;
    sut_ = prepareBalancerForNCpus(emptyCpuMap);
    sut_->setDomainPowerBudget(Limit{300.0, BudgetingStrategy::nonAggressive});

    EXPECT_CALL(*devManMock_, setKnobValue(testing::_, testing::_, testing::_))
        .Times(0);

    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, RunNeverUpdatesDevicesManagerIfNotChangedForNCpus)
{
    const std::vector<bool> cpuMap = {true,  true,  false, false,
                                      false, false, false, false};
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(std::nullopt);
    sut_->setComponentLimit(0, std::nullopt);
    sut_->setComponentLimit(1, std::nullopt);

    EXPECT_CALL(*devManMock_, setKnobValue(testing::_, testing::_, testing::_))
        .Times(0);
    // simulate few run() calls which never call setKnobValue()
    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DistributeLowBudgetWithComponentLimitForNCpus)
{
    const std::vector<bool> cpuMap = {true,  true,  false, true,
                                      false, false, true,  false};
    const auto strategy = BudgetingStrategy::nonAggressive;
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(Limit{100.0, strategy});
    sut_->setComponentLimit(0, Limit{40.0, strategy});
    sut_->setComponentLimit(1, Limit{50.0, strategy});
    sut_->setComponentLimit(3, Limit{60.0, strategy});
    sut_->setComponentLimit(7, Limit{70.0, strategy});

    std::vector<double> expectedHwLimits{25.0, 25.0,        kUnassigned,
                                         25.0, kUnassigned, kUnassigned,
                                         25.0, kUnassigned};
    for (unsigned deviceindex = 0; deviceindex < cpuMap.size(); deviceindex++)
    {
        if (cpuMap[deviceindex])
        {
            EXPECT_CALL(
                *devManMock_,
                setKnobValue(testing::_, deviceindex,
                             testing::DoubleEq(expectedHwLimits[deviceindex])));
        }
        else
        {
            EXPECT_CALL(*devManMock_,
                        setKnobValue(testing::_, deviceindex, testing::_))
                .Times(0);
        }
    }
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DistributeMediumBudgetWithComponentLimitForNCpus)
{
    const std::vector<bool> cpuMap = {false, true, false, true,
                                      false, true, true,  false};
    const auto strategy = BudgetingStrategy::nonAggressive;
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(Limit{200.0, strategy});
    sut_->setComponentLimit(1, Limit{40.0, strategy});
    sut_->setComponentLimit(3, Limit{50.0, strategy});
    sut_->setComponentLimit(5, Limit{60.0, strategy});
    sut_->setComponentLimit(6, Limit{70.0, strategy});
    // set below componentLimits which will be ignored as the CPU is inactive
    sut_->setComponentLimit(0, Limit{30.0, strategy});
    sut_->setComponentLimit(2, Limit{20.0, strategy});

    std::vector<double> expectedHwLimits{kUnassigned, 40.0,        kUnassigned,
                                         50.0,        kUnassigned, 55.0,
                                         55.0,        kUnassigned};
    for (unsigned deviceindex = 0; deviceindex < cpuMap.size(); deviceindex++)
    {
        if (cpuMap[deviceindex])
        {
            EXPECT_CALL(
                *devManMock_,
                setKnobValue(testing::_, deviceindex,
                             testing::DoubleEq(expectedHwLimits[deviceindex])));
        }
        else
        {
            EXPECT_CALL(*devManMock_,
                        setKnobValue(testing::_, deviceindex, testing::_))
                .Times(0);
        }
    }
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DistributeLargeBudgetWithComponentLimitForNCpus)
{
    const std::vector<bool> cpuMap = {true,  true,  false, true,
                                      false, false, true,  true};
    const auto strategy = BudgetingStrategy::nonAggressive;
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(Limit{500.0, strategy});
    sut_->setComponentLimit(0, Limit{40.0, strategy});
    sut_->setComponentLimit(1, Limit{50.0, strategy});
    sut_->setComponentLimit(3, Limit{60.0, strategy});
    sut_->setComponentLimit(6, Limit{70.0, strategy});
    sut_->setComponentLimit(7, Limit{80.0, strategy});

    std::vector<double> expectedHwLimits{
        40.0, 50.0, kUnassigned, 60.0, kUnassigned, kUnassigned, 70.0, 80.0};
    for (unsigned deviceindex = 0; deviceindex < cpuMap.size(); deviceindex++)
    {
        if (cpuMap[deviceindex])
        {
            EXPECT_CALL(
                *devManMock_,
                setKnobValue(testing::_, deviceindex,
                             testing::DoubleEq(expectedHwLimits[deviceindex])));
        }
        else
        {
            EXPECT_CALL(*devManMock_,
                        setKnobValue(testing::_, deviceindex, testing::_))
                .Times(0);
        }
    }
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DistributeNoLimitBudgetWithComponentLimitForNCpus)
{
    const std::vector<bool> cpuMap = {true, true, false, true,
                                      true, true, false, false};
    const auto strategy = BudgetingStrategy::nonAggressive;
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(std::nullopt);
    sut_->setComponentLimit(0, Limit{40.0, strategy});
    sut_->setComponentLimit(1, Limit{50.0, strategy});
    sut_->setComponentLimit(3, Limit{60.0, strategy});
    sut_->setComponentLimit(4, Limit{70.0, strategy});
    sut_->setComponentLimit(5, Limit{80.0, strategy});

    std::vector<double> expectedHwLimits{40.0, 50.0, kUnassigned, 60.0,
                                         70.0, 80.0, kUnassigned, kUnassigned};
    for (unsigned deviceindex = 0; deviceindex < cpuMap.size(); deviceindex++)
    {
        if (cpuMap[deviceindex])
        {
            EXPECT_CALL(
                *devManMock_,
                setKnobValue(testing::_, deviceindex,
                             testing::DoubleEq(expectedHwLimits[deviceindex])));
        }
        else
        {
            EXPECT_CALL(*devManMock_,
                        setKnobValue(testing::_, deviceindex, testing::_))
                .Times(0);
        }
    }
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DistributeNoLimitBudgetWithNoComponentLimitForNCpus)
{
    const auto strategy = BudgetingStrategy::nonAggressive;
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setDomainPowerBudget(std::nullopt);

    sut_->setComponentLimit(0, Limit{111.0, strategy});
    sut_->setComponentLimit(1, Limit{121.0, strategy});
    sut_->setComponentLimit(2, Limit{131.0, strategy});
    sut_->setComponentLimit(3, Limit{141.0, strategy});
    sut_->setComponentLimit(4, Limit{151.0, strategy});
    sut_->setComponentLimit(5, Limit{161.0, strategy});
    sut_->setComponentLimit(6, Limit{171.0, strategy});
    sut_->setComponentLimit(7, Limit{181.0, strategy});
    std::vector<double> expectedHwLimits{111.0, 121.0, 131.0, 141.0,
                                         151.0, 161.0, 171.0, 181.0};
    for (unsigned deviceindex = 0; deviceindex < cpuMap_.size(); deviceindex++)
    {
        EXPECT_CALL(
            *devManMock_,
            setKnobValue(testing::_, deviceindex,
                         testing::DoubleEq(expectedHwLimits[deviceindex])));
    }
    ASSERT_NO_THROW(sut_->run());

    sut_->setComponentLimit(0, std::nullopt);
    sut_->setComponentLimit(1, std::nullopt);
    sut_->setComponentLimit(2, std::nullopt);
    sut_->setComponentLimit(3, std::nullopt);
    sut_->setComponentLimit(4, std::nullopt);
    sut_->setComponentLimit(5, std::nullopt);
    sut_->setComponentLimit(6, std::nullopt);
    sut_->setComponentLimit(7, std::nullopt);

    expectedHwLimits = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    for (unsigned deviceindex = 0; deviceindex < cpuMap_.size(); deviceindex++)
    {
        EXPECT_CALL(*devManMock_, resetKnobValue(testing::_, deviceindex));
    }
    ASSERT_NO_THROW(sut_->run());
}

TEST_F(BalancerTest, DoNotSetDomainBudgetExpectDomainLimitIsNotActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS

    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == false);
}

TEST_F(BalancerTest, SetDomainBudgetExpectDomainLimitIsActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setDomainPowerBudget(Limit{800.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == true);
}

TEST_F(BalancerTest, SetDomainBudgetExpectDomainLimitIsActiveMultipleRun)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setDomainPowerBudget(Limit{800.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == true);
}

TEST_F(BalancerTest, RemoveDomainBudgetExpectDomainLimitIsNotActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS

    sut_->setDomainPowerBudget(Limit{800.0, BudgetingStrategy::nonAggressive});
    ASSERT_NO_THROW(sut_->run());

    sut_->setDomainPowerBudget(std::nullopt);
    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == false);
}

TEST_F(BalancerTest, SetComponentLimitExpectDomainLimitIsNotActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setComponentLimit(0, Limit{100.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == false);
}

TEST_F(BalancerTest,
       SetDomainBudgetSetLimitForSelectedComponentsExpectDomainLimitIsActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setDomainPowerBudget(Limit{800.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(0, Limit{10.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(2, Limit{10.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == true);
}

TEST_F(
    BalancerTest,
    SetDomainBudgetSetLessRestrictiveLimitForAllComponentsExpectDomainLimitIsActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setDomainPowerBudget(Limit{800.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(0, Limit{100.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(1, Limit{200.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(2, Limit{300.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(3, Limit{100.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(4, Limit{200.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(5, Limit{200.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(6, Limit{300.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(7, Limit{100.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == true);
}

TEST_F(
    BalancerTest,
    SetDomainBudgetSetMoreRestrictiveLimitForAllComponentsExpectDomainLimitIsNotActive)
{
    sut_ = prepareBalancerForNCpus(cpuMap_); // use default map with 8 CPUS
    sut_->setDomainPowerBudget(Limit{800.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(0, Limit{90.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(1, Limit{80.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(2, Limit{70.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(3, Limit{60.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(4, Limit{50.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(5, Limit{40.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(6, Limit{30.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(7, Limit{20.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());
    EXPECT_TRUE(sut_->isDomainLimitActive() == false);
}

TEST_F(
    BalancerTest,
    SetDomainBudgetSetMoreRestrictiveLimitForComponentExpectDomainLimitIsNotActive)
{
    const std::vector<bool> cpuMap = {true,  false, false, false,
                                      false, false, false, false};
    sut_ = prepareBalancerForNCpus(cpuMap);
    sut_->setDomainPowerBudget(Limit{100.0, BudgetingStrategy::nonAggressive});
    sut_->setComponentLimit(0, Limit{10.0, BudgetingStrategy::nonAggressive});

    ASSERT_NO_THROW(sut_->run());

    EXPECT_TRUE(sut_->isDomainLimitActive() == false);
}
