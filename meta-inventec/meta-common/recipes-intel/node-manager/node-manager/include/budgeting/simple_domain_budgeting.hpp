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

#include "budgeting/efficiency_helper.hpp"
#include "domains/capabilities/domain_capabilities.hpp"
#include "domains/capabilities/limit_capabilities.hpp"
#include "regulator_p.hpp"

#include <algorithm>

static constexpr double kLimitMultiplierUpper = 1.2;
static constexpr double kLimitMultiplierLower = -1.2;

namespace nodemanager
{

class SimpleDomainBudgetingIf
{
  public:
    virtual ~SimpleDomainBudgetingIf() = default;

    virtual double calculateDomainBudget(double) = 0;
    virtual void update(std::shared_ptr<DomainCapabilitiesIf>) = 0;
};

class SimpleDomainBudgeting : public SimpleDomainBudgetingIf
{
  public:
    SimpleDomainBudgeting() = delete;
    SimpleDomainBudgeting(const SimpleDomainBudgeting&) = delete;
    SimpleDomainBudgeting& operator=(const SimpleDomainBudgeting&) = delete;
    SimpleDomainBudgeting(SimpleDomainBudgeting&&) = delete;
    SimpleDomainBudgeting& operator=(SimpleDomainBudgeting&&) = delete;

    SimpleDomainBudgeting(
        std::unique_ptr<RegulatorIf> regulatorArg,
        std::unique_ptr<EfficiencyHelperIf> efficiencyHelperArg,
        double budgetCorrectionArg) :
        regulator(std::move(regulatorArg)),
        efficiencyHint(std::move(efficiencyHelperArg)),
        budgetCorrection(budgetCorrectionArg)
    {
    }

    virtual ~SimpleDomainBudgeting()
    {
    }

    virtual double calculateDomainBudget(double totalPowerBudget) override final
    {
        double hint = efficiencyHint->getHint();
        double controlSignal =
            regulator->calculateControlSignal(totalPowerBudget);
        const auto [min, max] =
            capabilities != nullptr
                ? std::make_pair(capabilities->getMin(), capabilities->getMax())
                : std::make_pair(minCapability, maxCapability);

        accControlSignal += controlSignal;
        accControlSignal =
            std::clamp(accControlSignal, kLimitMultiplierLower * max,
                       kLimitMultiplierUpper * max);

        accInternalBudget += budgetCorrection * hint;
        accInternalBudget =
            std::clamp(accInternalBudget, kLimitMultiplierLower * max,
                       kLimitMultiplierUpper * max);

        return std::clamp(accControlSignal + accInternalBudget, min, max);
    }

    virtual void update(std::shared_ptr<DomainCapabilitiesIf> newCapabilities)
    {
        if (newCapabilities)
        {
            capabilities = newCapabilities;
            accControlSignal = 0.0;
            accInternalBudget = capabilities->getMax();
        }
    }

  private:
    std::unique_ptr<RegulatorIf> regulator;
    std::unique_ptr<EfficiencyHelperIf> efficiencyHint;
    double minCapability{0.0};
    double maxCapability{kUnknownMaxPowerLimitInWatts};
    const double budgetCorrection;
    double accControlSignal{0.0};
    double accInternalBudget{maxCapability};
    std::shared_ptr<DomainCapabilitiesIf> capabilities;
};

} // namespace nodemanager
