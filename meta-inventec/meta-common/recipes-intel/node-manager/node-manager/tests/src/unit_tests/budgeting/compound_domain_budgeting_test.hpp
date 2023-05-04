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

#include "budgeting/compound_domain_budgeting.hpp"
#include "mocks/simple_domain_budgeting_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class CompoundDomainBudgetingTest : public ::testing::Test
{
  public:
    CompoundDomainBudgetingTest()
    {
    }

    virtual void SetUp() override
    {
        SimpleDomainDistributors distributors_;

        auto distributorMock_{
            std::make_unique<::testing::NiceMock<SimpleDomainBudgetingMock>>()};
        ON_CALL(*distributorMock_, calculateDomainBudget(testing::_))
            .WillByDefault(testing::Return(10.0));
        distributorsMocks_.emplace_back(distributorMock_.get());
        distributors_.emplace_back(RaplDomainId::memorySubsystem,
                                   std::move(distributorMock_));

        distributorMock_ =
            std::make_unique<::testing::NiceMock<SimpleDomainBudgetingMock>>();
        ON_CALL(*distributorMock_, calculateDomainBudget(testing::_))
            .WillByDefault(testing::Return(20.0));
        distributorsMocks_.emplace_back(distributorMock_.get());
        distributors_.emplace_back(RaplDomainId::pcie,
                                   std::move(distributorMock_));

        sut_ =
            std::make_unique<CompoundDomainBudgeting>(std::move(distributors_));
    }

  protected:
    std::vector<SimpleDomainBudgetingMock*> distributorsMocks_;
    std::unique_ptr<CompoundDomainBudgetingIf> sut_;
};

TEST_F(CompoundDomainBudgetingTest,
       VerifyTotalPowerBudgetDistributedToRaplLimits)
{
    double totalPoweburBudget_{111};

    for (auto distributorMock_ : distributorsMocks_)
    {
        EXPECT_CALL(*distributorMock_,
                    calculateDomainBudget(totalPoweburBudget_));
    }

    auto limits = sut_->distributeBudget(totalPoweburBudget_);

    EXPECT_THAT(limits, ::testing::ElementsAre(
                            std::pair{RaplDomainId::memorySubsystem, 10.0},
                            std::pair{RaplDomainId::pcie, 20.0},
                            std::pair{RaplDomainId::dcTotalPower,
                                      totalPoweburBudget_}));
}