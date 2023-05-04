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

#include "budgeting/budgeting.hpp"
#include "common_types.hpp"
#include "devices_manager/devices_manager.hpp"
#include "domains/capabilities/capabilities_factory.hpp"
#include "domains/domain.hpp"
#include "domains/domain_ac_total_power.hpp"
#include "domains/domain_cpu_subsystem.hpp"
#include "domains/domain_dc_total_power.hpp"
#include "domains/domain_hw_protection.hpp"
#include "domains/domain_memory_subsystem.hpp"
#include "domains/domain_pcie.hpp"
#include "domains/domain_performance.hpp"
#include "efficiency_control.hpp"
#include "flow_control.hpp"
#include "loggers/log.hpp"
#include "policies/policy_factory.hpp"
#include "triggers/triggers_manager.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/for_each.hpp"
#include "utility/state_if.hpp"
#include "utility/tag.hpp"

#include <iostream>
#include <type_traits>

namespace nodemanager
{

class Ptam : public RunnerExtIf, public StateIf
{
  public:
    Ptam() = delete;
    Ptam(const Ptam&) = delete;
    Ptam& operator=(const Ptam&) = delete;
    Ptam(Ptam&&) = delete;
    Ptam& operator=(Ptam&&) = delete;

    Ptam(const std::shared_ptr<sdbusplus::asio::connection>& busArg,
         const std::shared_ptr<sdbusplus::asio::object_server>& objectServerArg,
         std::string const& objectPathArg,
         const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
         const std::shared_ptr<GpioProviderIf>& gpioProviderArg,
         const std::shared_ptr<BudgetingIf>& budgetingArg,
         const std::shared_ptr<EfficiencyControlIf>& efficiencyControlArg,
         const std::shared_ptr<TriggersManagerIf>& triggersManagerArg,
         const std::shared_ptr<PolicyStorageManagementIf>&
             storageManagementArg) :
        bus(busArg),
        objectServer(objectServerArg), objectPath(objectPathArg),
        devicesManager(devicesManagerArg), gpioProvider(gpioProviderArg),
        budgeting(budgetingArg), efficiencyControl(efficiencyControlArg),
        triggersManager(triggersManagerArg),
        storageManagement(storageManagementArg),
        policyFactory(std::make_shared<PolicyFactory>(
            busArg, objectServerArg, triggersManagerArg, devicesManagerArg,
            gpioProviderArg, storageManagementArg)),
        capabilitiesFactory(
            std::make_shared<CapabilitiesFactory>(devicesManagerArg))
    {
        installDomains();
        createPoliciesFromPersistentMemory();
        createDefaultPolicies();
    }

    ~Ptam()
    {
    }

    virtual void setParentRunning(const bool value)
    {
        for (const auto& [domainId, domain] : domains)
        {
            domain->setParentRunning(value);
        }
    }

    std::shared_ptr<DomainCapabilitiesIf>
        getDomainCapabilities(DomainId id) const
    {
        try
        {
            return domains.at(id)->getDomainCapabilities();
        }
        catch (const boost::container::out_of_range&)
        {
            Logger::log<LogLevel::warning>(
                "Cannot get domain capabilities. Domain %s not supported",
                enumToStr(kDomainIdNames, id));
        }
        return nullptr;
    }

    void run() override final
    {
        auto perf = Perf("Ptam-run-duration", std::chrono::milliseconds{20});
        for (const auto& [domainId, domain] : domains)
        {
            domain->run();
        }

        // TODO: run PTAM logic here
    }

    void postRun() override final
    {
        auto perf =
            Perf("Ptam-postRun-duration", std::chrono::milliseconds{20});

        for (const auto& [domainId, domain] : domains)
        {
            domain->postRun();
        }
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string const& objectPath;
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::shared_ptr<BudgetingIf> budgeting;
    std::shared_ptr<EfficiencyControlIf> efficiencyControl;
    std::shared_ptr<TriggersManagerIf> triggersManager;
    std::shared_ptr<PolicyStorageManagementIf> storageManagement;
    boost::container::flat_map<DomainId, std::unique_ptr<Domain>> domains;
    std::shared_ptr<PolicyFactoryIf> policyFactory;
    std::shared_ptr<CapabilitiesFactory> capabilitiesFactory;

    void installDomains()
    {
        const auto& cfg = Config::getInstance().getGeneralPresets();
        auto domainsInfo = std::make_tuple(
            std::make_tuple(Tag<DomainCpuSubsystem>(),
                            cfg.cpuSubsystemDomainPresent,
                            cfg.cpuSubsystemDomainEnabled),
            std::make_tuple(Tag<DomainMemorySubsystem>(),
                            cfg.memorySubsystemDomainPresent,
                            cfg.memorySubsystemDomainEnabled),
            std::make_tuple(Tag<DomainHwProtection>(),
                            cfg.hwProtectionDomainPresent,
                            cfg.hwProtectionDomainEnabled),
            std::make_tuple(Tag<DomainAcTotalPower>(),
                            cfg.acTotalPowerDomainPresent,
                            cfg.acTotalPowerDomainEnabled),
            std::make_tuple(Tag<DomainDcTotalPower>(),
                            cfg.dcTotalPowerDomainPresent,
                            cfg.dcTotalPowerDomainEnabled),
            std::make_tuple(Tag<DomainPcie>(), cfg.pcieDomainPresent,
                            cfg.pcieDomainEnabled),
            std::make_tuple(Tag<DomainPerformance>(),
                            cfg.performanceDomainPresent,
                            cfg.performanceDomainEnabled));

        domains.reserve(std::tuple_size_v<decltype(domainsInfo)>);

        for_each(domainsInfo, [this](auto tag, auto isPresent, auto isEnabled) {
            if (!isPresent)
            {
                return;
            }
            if constexpr (std::is_base_of_v<DomainPower,
                                            typename decltype(tag)::type>)
            {
                domains[decltype(tag)::type::ID] =
                    std::make_unique<typename decltype(tag)::type>(
                        bus, objectServer, objectPath, devicesManager,
                        gpioProvider, triggersManager, budgeting, policyFactory,
                        capabilitiesFactory,
                        (isEnabled ? DbusState::enabled : DbusState::disabled));
            }
            else if constexpr (std::is_base_of_v<DomainPerformance,
                                                 typename decltype(tag)::type>)
            {
                domains[decltype(tag)::type::ID] =
                    std::make_unique<typename decltype(tag)::type>(
                        bus, objectServer, objectPath, devicesManager,
                        gpioProvider, triggersManager, efficiencyControl,
                        policyFactory, capabilitiesFactory,
                        (isEnabled ? DbusState::enabled : DbusState::disabled));
            }
        });
    }

    void createPoliciesFromPersistentMemory()
    {
        for (auto [policyId, domainId, owner, isEnabled, policyParams] :
             storageManagement->policiesRead())
        {
            try
            {
                domains.at(domainId)->createPolicy(
                    policyId, owner, policyParams, kForcedCreate,
                    isEnabled ? DbusState::enabled : DbusState::disabled,
                    PolicyEditable::yes, true);
            }
            catch (const boost::container::out_of_range&)
            {
                Logger::log<LogLevel::warning>(
                    "Cannot create policy with id %s from file. Domain %s not "
                    "supported",
                    policyId, enumToStr(kDomainIdNames, domainId));
                storageManagement->policyDelete(policyId);
            }
            catch (const std::exception& e)
            {
                Logger::log<LogLevel::warning>(
                    "Cannot create policy with id %s from file, reason: %s",
                    policyId, e.what());
                storageManagement->policyDelete(policyId);
            }
        }
    }

    void createDefaultPolicies()
    {
        for (const auto& [domainId, domain] : domains)
        {
            try
            {
                domain->createDefaultPolicies();
            }
            catch (const std::exception& e)
            {
                Logger::log<LogLevel::warning>(
                    "Cannot create default policy for domain %s, reason: %s",
                    enumToStr(kDomainIdNames, domainId), e.what());
            }
            catch (...)
            {
                Logger::log<LogLevel::warning>(
                    "Cannot create default policy for domain %s, reason: "
                    "unknown",
                    enumToStr(kDomainIdNames, domainId));
            }
        }
    }
};

} // namespace nodemanager
