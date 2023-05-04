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

#include "domains/capabilities/domain_capabilities.hpp"
#include "domains/domain_types.hpp"
#include "policies/performance_policy.hpp"
#include "policies/policy.hpp"
#include "policies/policy_enums.hpp"
#include "policies/policy_types.hpp"
#include "policies/power_policy.hpp"
#include "readings/reading_type.hpp"
#include "utility/dbus_enable_if.hpp"

#include <memory>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

namespace nodemanager
{

class PolicyFactoryIf
{
  public:
    virtual ~PolicyFactoryIf() = default;

    virtual std::shared_ptr<PolicyIf> createPolicy(
        std::shared_ptr<DomainInfo>&, PolicyType, PolicyId, PolicyOwner,
        uint16_t, DeleteCallback, DbusState, PolicyEditable, bool,
        std::shared_ptr<KnobCapabilitiesIf>,
        const std::vector<std::shared_ptr<ComponentCapabilitiesIf>>&) = 0;
};

class PolicyFactory : public PolicyFactoryIf
{
  public:
    PolicyFactory(const PolicyFactory&) = delete;
    PolicyFactory& operator=(const PolicyFactory&) = delete;
    PolicyFactory(PolicyFactory&&) = delete;
    PolicyFactory& operator=(PolicyFactory&&) = delete;

    PolicyFactory(
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<sdbusplus::asio::object_server>& objectServerArg,
        const std::shared_ptr<TriggersManagerIf>& triggersManagerArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        const std::shared_ptr<GpioProviderIf>& gpioProviderArg,
        const std::shared_ptr<PolicyStorageManagementIf>&
            storageManagementArg) :
        bus(busArg),
        objectServer(objectServerArg), triggersManager(triggersManagerArg),
        devicesManager(devicesManagerArg), gpioProvider(gpioProviderArg),
        storageManagement(storageManagementArg)
    {
    }

    std::shared_ptr<PolicyIf> createPolicy(
        std::shared_ptr<DomainInfo>& domainInfo, PolicyType policyType,
        PolicyId pId, PolicyOwner policyOwner, uint16_t statReportingPeriod,
        DeleteCallback deleteCallbackArg, DbusState dbusState,
        PolicyEditable editable, bool allowDelete,
        std::shared_ptr<KnobCapabilitiesIf> knobCapabilities,
        const std::vector<std::shared_ptr<ComponentCapabilitiesIf>>&
            componentCapabilitiesVector) override
    {
        if (idAlreadyExists(pId, powerPolicies) ||
            idAlreadyExists(pId, performancePolicies))
        {
            Logger::log<LogLevel::warning>("Policy with id: %s already exists",
                                           pId);
            throw errors::PoliciesCannotBeCreated();
        }

        switch (policyType)
        {
            case PolicyType::power: {
                removeInvalidPointers(powerPolicies);

                if (PolicyOwner::bmc == policyOwner &&
                    !isBelowMaxPowerPolicyLimit())
                {
                    Logger::log<LogLevel::warning>("Max Policy limit reached");
                    throw errors::PoliciesCannotBeCreated();
                }

                auto policy = std::make_shared<PowerPolicy>(
                    pId, policyOwner, devicesManager, gpioProvider,
                    triggersManager, storageManagement, statReportingPeriod,
                    bus, objectServer, domainInfo, std::move(deleteCallbackArg),
                    dbusState, editable, allowDelete,
                    componentCapabilitiesVector);

                powerPolicies.emplace_back(policy);

                return policy;
            }
            case PolicyType::performance: {
                removeInvalidPointers(performancePolicies);

                auto policy = std::make_shared<PerformancePolicy>(
                    pId, policyOwner, devicesManager, gpioProvider,
                    triggersManager, storageManagement, statReportingPeriod,
                    bus, objectServer, domainInfo, std::move(deleteCallbackArg),
                    dbusState, editable, allowDelete, knobCapabilities);

                performancePolicies.emplace_back(policy);

                return policy;
            }
            default:
                throw std::runtime_error("Unknown Policy type");
        }

        return nullptr;
    }

  private:
    void removeInvalidPointers(std::vector<std::weak_ptr<PolicyIf>>& policies)
    {
        policies.erase(std::remove_if(policies.begin(), policies.end(),
                                      [](const std::weak_ptr<PolicyIf>& wp) {
                                          return wp.expired();
                                      }),
                       policies.end());
    }

    bool idAlreadyExists(const PolicyId& id,
                         std::vector<std::weak_ptr<PolicyIf>>& policies)
    {
        return std::any_of(policies.cbegin(), policies.cend(),
                           [this, &id](const std::weak_ptr<PolicyIf>& wp) {
                               if (auto sp = wp.lock())
                               {
                                   return sp->getId() == id;
                               }
                               return false;
                           });
    }

    bool isBelowMaxPowerPolicyLimit()
    {
        return std::count_if(powerPolicies.cbegin(), powerPolicies.cend(),
                             [this](const std::weak_ptr<PolicyIf>& wp) {
                                 if (auto sp = wp.lock())
                                 {
                                     return sp->getOwner() == PolicyOwner::bmc;
                                 }
                                 return false;
                             }) < kNodeManagerMaxPolicies;
    }
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::shared_ptr<TriggersManagerIf> triggersManager;
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::shared_ptr<PolicyStorageManagementIf> storageManagement;
    std::vector<std::weak_ptr<PolicyIf>> powerPolicies;
    std::vector<std::weak_ptr<PolicyIf>> performancePolicies;
};

} // namespace nodemanager
