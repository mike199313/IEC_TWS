/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2021 Intel Corporation.
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
#include "compound_domain_budgeting.hpp"
#include "control/control.hpp"
#include "devices_manager/devices_manager.hpp"
#include "flow_control.hpp"
#include "power_limit_selector.hpp"
#include "readings/reading_event.hpp"
#include "utility/performance_monitor.hpp"

#include <iostream>

namespace nodemanager
{

static constexpr const double kDefaultPsuEfficiency = 1.0;
class BudgetingIf : public RunnerIf
{
  public:
    virtual ~BudgetingIf() = default;
    virtual void setLimit(DomainId domainId, DeviceIndex componentId,
                          double limitValue,
                          BudgetingStrategy budgetingStrategy) = 0;
    virtual void resetLimit(DomainId domainId, DeviceIndex componentId,
                            BudgetingStrategy budgetingStrategy) = 0;
    virtual bool isActive(DomainId domainId, DeviceIndex componentId,
                          BudgetingStrategy budgetingStrategy) = 0;
};

class Budgeting : public BudgetingIf
{
    using PtamLimit = std::optional<double>;

  public:
    Budgeting() = delete;
    Budgeting(const Budgeting&) = delete;
    Budgeting& operator=(const Budgeting&) = delete;
    Budgeting(Budgeting&&) = delete;
    Budgeting& operator=(Budgeting&&) = delete;

    Budgeting(std::shared_ptr<DevicesManagerIf> devicesManagerArg,
              std::unique_ptr<CompoundDomainBudgetingIf> compoundBudgetingArg,
              std::shared_ptr<ControlIf> controlArg) :
        devicesManager(devicesManagerArg),
        compoundBudgeting(std::move(compoundBudgetingArg)), control(controlArg),
        psuEfficiency(kDefaultPsuEfficiency)
    {
        installLimitSelectors();

        devicesManager->registerReadingConsumer(
            psuEfficiencyHandler, ReadingType::platformPowerEfficiency,
            kAllDevices);
    }

    virtual ~Budgeting()
    {
        devicesManager->unregisterReadingConsumer(psuEfficiencyHandler);
    }

    void run() override final
    {
        auto perf =
            Perf("Budgeting-run-duration", std::chrono::milliseconds{20});

        propagatePtamLimits();
        runCompoundBudgeting();
        selectRaplLimits();
    }

    void updateSimpleDomainBudgeting(
        const SimpleDomainCapabilities& capabilities)
    {
        compoundBudgeting->updateDistributors(capabilities);
    }

    virtual void setLimit(DomainId domainId, DeviceIndex componentId,
                          double limitValue,
                          BudgetingStrategy budgetingStrategy) final
    {
        Logger::log<LogLevel::debug>(
            "[Budgeting] setLimit for domainId %d "
            "componentId %d limit %d strategy %d \n",
            static_cast<unsigned>(domainId), static_cast<unsigned>(componentId),
            limitValue, static_cast<unsigned>(budgetingStrategy));

        if (componentId == kComponentIdAll)
        {
            ptamLimits[domainId][budgetingStrategy] = limitValue;
        }
        else
        {
            control->setComponentBudget(mapPtamDomainToRaplDomain(domainId),
                                        componentId,
                                        Limit{limitValue, budgetingStrategy});
        }
    }

    virtual void resetLimit(DomainId domainId, DeviceIndex componentId,
                            BudgetingStrategy budgetingStrategy) final
    {
        Logger::log<LogLevel::info>("[Budgeting] resetLimit for domainId %d "
                                    "componentId %d strategy %d \n",
                                    static_cast<unsigned>(domainId),
                                    static_cast<unsigned>(componentId),
                                    static_cast<unsigned>(budgetingStrategy));

        if (componentId == kComponentIdAll)
        {
            ptamLimits[domainId][budgetingStrategy].reset();
        }
        else
        {
            control->setComponentBudget(mapPtamDomainToRaplDomain(domainId),
                                        componentId, std::nullopt);
        }
    }

    virtual bool isActive(DomainId domainId, DeviceIndex componentId,
                          BudgetingStrategy budgetingStrategy) final
    {
        RaplDomainId raplDomainId = mapPtamDomainToRaplDomain(domainId);

        if (componentId == kComponentIdAll)
        {
            try
            {
                return limitSelectors.at(raplDomainId)
                           ->isActive(domainId, budgetingStrategy) &&
                       control->isDomainLimitActive(raplDomainId);
            }
            catch (const std::exception& e)
            {
                Logger::log<LogLevel::error>(
                    "[Budgeting]: domain [%u] is not supported: %s",
                    static_cast<unsigned>(domainId), e.what());
            }
        }
        else
        {
            return control->isComponentLimitActive(raplDomainId, componentId);
        }
        return false;
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::unique_ptr<CompoundDomainBudgetingIf> compoundBudgeting;
    std::shared_ptr<ControlIf> control;

    std::unordered_map<RaplDomainId, std::unique_ptr<PowerLimitSelector>>
        limitSelectors;
    std::unique_ptr<PowerLimitSelector> compoundLimitSelector;
    std::unordered_map<DomainId, std::map<BudgetingStrategy, PtamLimit>>
        ptamLimits;

    double psuEfficiency;
    std::shared_ptr<ReadingEvent> psuEfficiencyHandler =
        std::make_shared<ReadingEvent>([this](double incomingValue) {
            if (std::isnan(incomingValue))
            {
                psuEfficiency = kDefaultPsuEfficiency;
            }
            else
            {
                psuEfficiency = incomingValue;
            }
        });

    double convertPowerLimit(DomainId domainId, double limitValue)
    {
        if (domainId != DomainId::AcTotalPower)
        {
            return limitValue;
        }
        else
        {
            return limitValue * psuEfficiency;
        }
    }

    RaplDomainId mapPtamDomainToRaplDomain(DomainId domainId)
    {
        switch (domainId)
        {
            case DomainId::AcTotalPower:
            case DomainId::HwProtection:
            case DomainId::DcTotalPower:
                return RaplDomainId::dcTotalPower;
            case DomainId::CpuSubsystem:
                return RaplDomainId::cpuSubsystem;
            case DomainId::MemorySubsystem:
                return RaplDomainId::memorySubsystem;
            case DomainId::Pcie:
                return RaplDomainId::pcie;
            default:
                throw "Cannot convert Ptam domain to Rapl domain";
        }
    }

    PowerLimitSelector& getLimitSelector(DomainId domainId,
                                         BudgetingStrategy strategy)
    {
        if (strategy == BudgetingStrategy::aggressive &&
            (domainId == DomainId::AcTotalPower ||
             domainId == DomainId::DcTotalPower))
        {
            return *compoundLimitSelector;
        }
        else
        {
            try
            {
                return *limitSelectors.at(mapPtamDomainToRaplDomain(domainId));
            }
            catch (const std::exception& e)
            {
                Logger::log<LogLevel::error>(
                    "[Budgeting]: domain [%u] is not supported: %s",
                    static_cast<unsigned>(domainId), e.what());

                throw e;
            }
        }
    }

    void propagatePtamLimits()
    {
        resetLimitSelectors();

        for (auto& [domainId, limits] : ptamLimits)
        {
            for (auto& [strategy, ptamLimit] : limits)
            {
                if (!ptamLimit.has_value())
                {
                    continue;
                }

                Limit limit{convertPowerLimit(domainId, ptamLimit.value()),
                            strategy};

                getLimitSelector(domainId, strategy)
                    .updateLimit(limit, domainId);
            }
        }
    }

    void runCompoundBudgeting()
    {
        auto totalPowerBudget{compoundLimitSelector->getLimit()};
        auto sourceDomainId{compoundLimitSelector->getSourceDomain()};

        if (sourceDomainId.has_value() && totalPowerBudget.has_value())
        {
            auto raplLimits{compoundBudgeting->distributeBudget(
                totalPowerBudget.value().value)};

            for (const auto& [raplDomainId, limitValue] : raplLimits)
            {
                limitSelectors.at(raplDomainId)
                    ->updateLimit(
                        Limit{limitValue, totalPowerBudget.value().strategy},
                        sourceDomainId.value());
            }
        }
    }

    void selectRaplLimits()
    {
        for (const auto& [raplDomainId, limitSelector] : limitSelectors)
        {
            control->setBudget(raplDomainId, limitSelector->getLimit());
        }
    }

    void installLimitSelectors()
    {
        limitSelectors.emplace(RaplDomainId::dcTotalPower,
                               std::make_unique<PowerLimitSelector>());
        limitSelectors.emplace(RaplDomainId::cpuSubsystem,
                               std::make_unique<PowerLimitSelector>());
        limitSelectors.emplace(RaplDomainId::memorySubsystem,
                               std::make_unique<PowerLimitSelector>());
        limitSelectors.emplace(RaplDomainId::pcie,
                               std::make_unique<PowerLimitSelector>());

        compoundLimitSelector = std::make_unique<PowerLimitSelector>();
    }

    void resetLimitSelectors()
    {
        for (auto const& [raplDomainId, limitSelector] : limitSelectors)
        {
            limitSelector->resetLimit();
        }
        compoundLimitSelector->resetLimit();
    }
};

} // namespace nodemanager
