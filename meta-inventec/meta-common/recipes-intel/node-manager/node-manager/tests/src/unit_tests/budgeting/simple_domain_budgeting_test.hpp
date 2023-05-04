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

#include "budgeting/simple_domain_budgeting.hpp"
#include "mocks/domain_capabilities_mock.hpp"
#include "mocks/efficiency_helper_mock.hpp"
#include "mocks/regulator_mock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static const double kBudgetCorrection = 2;
double kMinCapDomain = 0.0;
double kMaxCapDomain = kUnknownMaxPowerLimitInWatts;
double kUpdatedMinCapDomain = 10.0;
double kUpdatedMaxCapDomain = 5000.0;
double kIterAccBudget = 4;
double kInitAccInternalBudget = kMaxCapDomain;

constexpr double getLimitUpperBudget(double max)
{
    return kLimitMultiplierUpper * max;
}

constexpr double getLimitLowerBudget(double max)
{
    return kLimitMultiplierLower * max;
}

class SimpleDomainBudgetingTest : public ::testing::Test
{
  public:
    SimpleDomainBudgetingTest()
    {
    }

    virtual void SetUp() override
    {
        auto regulator_{std::make_unique<::testing::NiceMock<RegulatorMock>>()};
        pRegulator_ = regulator_.get();

        auto efficiencyHint_{
            std::make_unique<::testing::NiceMock<EfficiencyHelperMock>>()};
        pEfficiencyHint_ = efficiencyHint_.get();

        sut_ = std::make_unique<SimpleDomainBudgeting>(
            std::move(regulator_), std::move(efficiencyHint_),
            kBudgetCorrection);
    }

  protected:
    RegulatorMock* pRegulator_;
    EfficiencyHelperMock* pEfficiencyHint_;
    std::unique_ptr<SimpleDomainBudgetingIf> sut_;
};

struct SignalsAndDomaninBudget
{
    double controlSignal;
    double hint;
    double domainBudget;
};

class SimpleDomainBudgetingTestWithParams
    : public SimpleDomainBudgetingTest,
      public testing::WithParamInterface<SignalsAndDomaninBudget>
{
};

class SimpleDomainBudgetingTestIterationWithParams
    : public SimpleDomainBudgetingTest,
      public testing::WithParamInterface<SignalsAndDomaninBudget>
{
};

class SimpleDomainBudgetingTestWithParamsForUpdatedCaps
    : public SimpleDomainBudgetingTest,
      public testing::WithParamInterface<SignalsAndDomaninBudget>
{
  public:
    SimpleDomainBudgetingTestWithParamsForUpdatedCaps() = default;
    virtual ~SimpleDomainBudgetingTestWithParamsForUpdatedCaps() = default;

    void SetUp() override
    {
        ON_CALL(*this->capabilities_, getMin())
            .WillByDefault(testing::Return(kUpdatedMinCapDomain));
        ON_CALL(*this->capabilities_, getMax())
            .WillByDefault(testing::Return(kUpdatedMaxCapDomain));

        SimpleDomainBudgetingTest::SetUp();
        sut_->update(capabilities_);
    }

  private:
    std::shared_ptr<DomainCapabilitiesMock> capabilities_ =
        std::make_shared<testing::NiceMock<DomainCapabilitiesMock>>();
};

INSTANTIATE_TEST_SUITE_P(
    BudgetDomainExpected, SimpleDomainBudgetingTestWithParams,
    ::testing::Values(
        SignalsAndDomaninBudget{0.0, 1.0, kMaxCapDomain},
        SignalsAndDomaninBudget{0.0, 0.0, kMaxCapDomain},
        SignalsAndDomaninBudget{0.0, -1.0,
                                kMaxCapDomain - kBudgetCorrection * 1.0},

        SignalsAndDomaninBudget{0.0, getLimitLowerBudget(kMaxCapDomain),
                                kMinCapDomain},

        SignalsAndDomaninBudget{1.0, (-1) * kMaxCapDomain / kBudgetCorrection,
                                (1.0 > kMinCapDomain) ? 1.0 : kMinCapDomain},

        SignalsAndDomaninBudget{-1.0, (-1) * kMaxCapDomain / kBudgetCorrection,
                                (-1.0 <= kMinCapDomain) ? kMinCapDomain : -1.0},
        SignalsAndDomaninBudget{getLimitUpperBudget(kMaxCapDomain),
                                (-1) * kMaxCapDomain / kBudgetCorrection,
                                kMaxCapDomain},

        SignalsAndDomaninBudget{getLimitLowerBudget(kMaxCapDomain),
                                (-1) * kMaxCapDomain / kBudgetCorrection,
                                kMinCapDomain}));

INSTANTIATE_TEST_SUITE_P(
    BudgetDomainExpected, SimpleDomainBudgetingTestIterationWithParams,
    ::testing::Values(
        // clamping only for Internal Budget
        SignalsAndDomaninBudget{0.0, getLimitUpperBudget(kMaxCapDomain),
                                kMaxCapDomain},
        SignalsAndDomaninBudget{0.0, getLimitLowerBudget(kMaxCapDomain),
                                kMinCapDomain},

        // clamping only for Control Signal
        SignalsAndDomaninBudget{getLimitUpperBudget(kMaxCapDomain) + 1.0,
                                getLimitLowerBudget(kMaxCapDomain),
                                kMinCapDomain},
        SignalsAndDomaninBudget{getLimitLowerBudget(kMaxCapDomain) - 1.0,
                                getLimitLowerBudget(kMaxCapDomain),
                                kMinCapDomain},

        // clamping budget domin to max capability
        SignalsAndDomaninBudget{100.0, 0.0, kMaxCapDomain},

        // clamping budget domin to min capability
        SignalsAndDomaninBudget{getLimitLowerBudget(kMaxCapDomain) - 1.0, 0.0,
                                kMinCapDomain},
        SignalsAndDomaninBudget{
            getLimitLowerBudget(kMaxCapDomain) / 3, 0.0,
            ((kInitAccInternalBudget +
              kIterAccBudget * getLimitLowerBudget(kMaxCapDomain) / 3) <=
             kMinCapDomain)
                ? kMinCapDomain
                : (kInitAccInternalBudget +
                   kIterAccBudget * getLimitLowerBudget(kMaxCapDomain) / 3)},

        SignalsAndDomaninBudget{
            getLimitLowerBudget(kMaxCapDomain) / 3, -1.0,
            ((kInitAccInternalBudget +
              kIterAccBudget * (kBudgetCorrection * -1.0 +
                                getLimitLowerBudget(kMaxCapDomain) / 3)) <=
             kMinCapDomain)
                ? kMinCapDomain
                : (kInitAccInternalBudget +
                   kIterAccBudget * (kBudgetCorrection * -1.0 +
                                     getLimitLowerBudget(kMaxCapDomain) / 3))},

        // testing range
        SignalsAndDomaninBudget{
            getLimitLowerBudget(kMaxCapDomain) / 10, -1.0,
            ((kInitAccInternalBudget +
              kIterAccBudget * (kBudgetCorrection * (-1.0) +
                                getLimitLowerBudget(kMaxCapDomain) / 10)) >=
             kMaxCapDomain)
                ? kMaxCapDomain
                : (kInitAccInternalBudget +
                   kIterAccBudget *
                       (kBudgetCorrection * (-1.0) +
                        getLimitLowerBudget(kMaxCapDomain) / 10))}));

INSTANTIATE_TEST_SUITE_P(
    BudgetDomainExpected, SimpleDomainBudgetingTestWithParamsForUpdatedCaps,
    ::testing::Values(
        SignalsAndDomaninBudget{0.0, 1.0, kUpdatedMaxCapDomain},
        SignalsAndDomaninBudget{0.0, 0.0, kUpdatedMaxCapDomain},
        SignalsAndDomaninBudget{0.0, -1.0,
                                kUpdatedMaxCapDomain - kBudgetCorrection * 1.0},
        SignalsAndDomaninBudget{0.0, getLimitLowerBudget(kUpdatedMaxCapDomain),
                                kUpdatedMinCapDomain},
        SignalsAndDomaninBudget{
            1.0, (-1) * kUpdatedMaxCapDomain / kBudgetCorrection,
            (1.0 > kUpdatedMinCapDomain) ? 1.0 : kUpdatedMinCapDomain},
        SignalsAndDomaninBudget{
            -1.0, (-1) * kUpdatedMaxCapDomain / kBudgetCorrection,
            (-1.0 <= kUpdatedMinCapDomain) ? kUpdatedMinCapDomain : -1.0},
        SignalsAndDomaninBudget{getLimitUpperBudget(kUpdatedMaxCapDomain),
                                (-1) * kUpdatedMaxCapDomain / kBudgetCorrection,
                                kUpdatedMaxCapDomain},
        SignalsAndDomaninBudget{getLimitLowerBudget(kUpdatedMaxCapDomain),
                                (-1) * kUpdatedMaxCapDomain / kBudgetCorrection,
                                kUpdatedMinCapDomain}));

TEST_P(SimpleDomainBudgetingTestWithParams, ExpectCorrectDomainBudget)
{
    double totalPowerBudget{1.0};
    auto [controlSignal, hint, domainBudget] = GetParam();

    ON_CALL(*pEfficiencyHint_, getHint()).WillByDefault(testing::Return(hint));
    EXPECT_CALL(*pRegulator_, calculateControlSignal(totalPowerBudget))
        .WillOnce(testing::Return(controlSignal));

    EXPECT_EQ(sut_->calculateDomainBudget(totalPowerBudget), domainBudget);
}

TEST_P(SimpleDomainBudgetingTestIterationWithParams,
       ExpectCorrectDomainBudgetAfterAccumulation)
{
    double totalPowerBudget{1.0};
    auto [controlSignal, hint, domainBudget] = GetParam();

    EXPECT_CALL(*pEfficiencyHint_, getHint())
        .Times(kIterAccBudget)
        .WillRepeatedly(testing::Return(hint));
    EXPECT_CALL(*pRegulator_, calculateControlSignal(totalPowerBudget))
        .Times(kIterAccBudget)
        .WillRepeatedly(testing::Return(controlSignal));

    for (int i = 0; i < (kIterAccBudget - 1); i++)
    {
        sut_->calculateDomainBudget(totalPowerBudget);
    }
    EXPECT_EQ(sut_->calculateDomainBudget(totalPowerBudget), domainBudget);
}

TEST_P(SimpleDomainBudgetingTestWithParamsForUpdatedCaps,
       ExpectCorrectDomainBudgetOnUpdatedCaps)
{
    double totalPowerBudget{1.0};
    auto [controlSignal, hint, domainBudget] = GetParam();

    ON_CALL(*pEfficiencyHint_, getHint()).WillByDefault(testing::Return(hint));
    EXPECT_CALL(*pRegulator_, calculateControlSignal(totalPowerBudget))
        .WillOnce(testing::Return(controlSignal));

    EXPECT_EQ(sut_->calculateDomainBudget(totalPowerBudget), domainBudget);
}

TEST_F(SimpleDomainBudgetingTest, ControlSignalIsNan_NanDomainBudgetExpected)
{
    double totalPowerBudget{1.0};

    ON_CALL(*pEfficiencyHint_, getHint()).WillByDefault(testing::Return(2.0));
    ON_CALL(*pRegulator_, calculateControlSignal(totalPowerBudget))
        .WillByDefault(
            testing::Return(std::numeric_limits<double>::quiet_NaN()));

    EXPECT_TRUE(std::isnan(sut_->calculateDomainBudget(totalPowerBudget)));
}