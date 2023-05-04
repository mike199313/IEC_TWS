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

#include "common_types.hpp"
#include "simple_domain_budgeting.hpp"

namespace nodemanager
{

using SimpleDomainDistributor =
    std::pair<RaplDomainId, std::unique_ptr<SimpleDomainBudgetingIf>>;

using SimpleDomainDistributors = std::vector<SimpleDomainDistributor>;

using SimpleDomainCapabilities =
    std::unordered_map<RaplDomainId, std::shared_ptr<DomainCapabilitiesIf>>;

class CompoundDomainBudgetingIf
{
  public:
    virtual ~CompoundDomainBudgetingIf() = default;
    virtual std::vector<std::pair<RaplDomainId, double>>
        distributeBudget(double totalPowerBudget) const = 0;
    virtual void updateDistributors(const SimpleDomainCapabilities&) = 0;
};

class CompoundDomainBudgeting : public CompoundDomainBudgetingIf
{

  public:
    CompoundDomainBudgeting() = delete;
    CompoundDomainBudgeting(const CompoundDomainBudgeting&) = delete;
    CompoundDomainBudgeting& operator=(const CompoundDomainBudgeting&) = delete;
    CompoundDomainBudgeting(CompoundDomainBudgeting&&) = delete;
    CompoundDomainBudgeting& operator=(CompoundDomainBudgeting&&) = delete;

    CompoundDomainBudgeting(SimpleDomainDistributors distributorsArg) :
        distributors(std::move(distributorsArg))
    {
    }

    std::vector<std::pair<RaplDomainId, double>>
        distributeBudget(double totalPowerBudget) const final
    {
        std::vector<std::pair<RaplDomainId, double>> raplLimits{};

        for (auto& [raplDomainId, distributor] : distributors)
        {
            raplLimits.emplace_back(
                raplDomainId,
                distributor->calculateDomainBudget(totalPowerBudget));
        }

        raplLimits.emplace_back(RaplDomainId::dcTotalPower, totalPowerBudget);

        return raplLimits;
    }

    void updateDistributors(
        const SimpleDomainCapabilities& simpleDomainCapabilities)
    {
        for (auto& [raplDomainId, capabilities] : simpleDomainCapabilities)
        {
            if (auto it = std::ranges::find(distributors, raplDomainId,
                                            &SimpleDomainDistributor::first);
                it != distributors.end())
            {
                auto& [_, simpleDomainBudgeting] = *it;
                simpleDomainBudgeting->update(capabilities);
            }
            else
            {
                Logger::log<LogLevel::warning>(
                    "Cannot update SimpleDomainCapabilities, missing "
                    "SimpleDomainBudgeting for requested rapl domain: %d",
                    static_cast<unsigned>(raplDomainId));
            }
        }
    }

  private:
    SimpleDomainDistributors distributors;
}; // class CompoundDomainBudgeting

} // namespace nodemanager