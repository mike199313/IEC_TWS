/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2022 Intel Corporation.
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
#include "flow_control.hpp"
#include "loggers/log.hpp"
#include "policies/policy_enums.hpp"
#include "policies/policy_factory.hpp"
#include "policies/policy_types.hpp"
#include "statistics/statistics_provider.hpp"
#include "utility/dbus_enable_if.hpp"
#include "utility/state_if.hpp"

#include <iostream>

namespace nodemanager
{

static constexpr int16_t kZeroTriggerLimit = 0;
static constexpr bool kForcedCreate = true;
static constexpr bool kNotForcedCreate = false;
static constexpr double kMaxSupportedCapabilityValue = double{0x7FFF};
static constexpr double kMinSupportedCapabilityValue = 0.0;

class Domain : public RunnerExtIf,
               public DbusEnableIf,
               public StatisticsProvider
{
  public:
    Domain() = delete;
    Domain(const Domain&) = delete;
    Domain& operator=(const Domain&) = delete;
    Domain(Domain&&) = delete;
    Domain& operator=(Domain&&) = delete;

    Domain(
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<sdbusplus::asio::object_server>& objectServerArg,
        std::string const& objectPathArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        const std::shared_ptr<GpioProviderIf>& gpioProviderArg,
        const std::shared_ptr<TriggersManagerIf>& triggersManagerArg,
        DomainId idArg,
        const std::shared_ptr<PolicyFactoryIf>& policyFactoryArg,
        const std::shared_ptr<DomainInfo>& domainInfoArg,
        const DbusState dbusState) :
        DbusEnableIf(dbusState),
        StatisticsProvider(devicesManagerArg), bus(busArg),
        objectServer(objectServerArg), devicesManager(devicesManagerArg),
        gpioProvider(gpioProviderArg), triggersManager(triggersManagerArg),
        policyFactory(policyFactoryArg), domainInfo(domainInfoArg)
    {
        DbusEnableIf::initializeDbusInterfaces(dbusInterfaces);
        StatisticsProvider::initializeDbusInterfaces(dbusInterfaces);
        initializeDbusInterfaces();
    }

    virtual ~Domain() = default;

    virtual void onStateChanged() override
    {
        Logger::log<LogLevel::info>("Domain: %d, is running: %b",
                                    static_cast<int>(domainInfo->domainId),
                                    isRunning());

        for (auto const& policy : policies)
        {
            policy->setParentRunning(isRunning());
        }
    }

    virtual void run() override
    {
        for (auto const& policy : policies)
        {
            policy->run();
        }
    }

    virtual void postRun() override
    {
    }

    virtual void createDefaultPolicies()
    {
    }

    std::shared_ptr<PolicyIf>
        createPolicy(PolicyId pId, const PolicyOwner policyOwner,
                     const PolicyParams& params, const bool force,
                     const DbusState dbusState, const PolicyEditable editable,
                     const bool allowDelete)
    {
        auto policy = createPolicyFromFactory(
            pId, policyOwner, params.statReportingPeriod, force, dbusState,
            editable, allowDelete, [this](const PolicyId policyId) {
                for (auto it = policies.cbegin(); it != policies.cend(); it++)
                {
                    if ((*it)->getId() == policyId)
                    {
                        Logger::log<LogLevel::info>(
                            "Number of references to Policy: %s before "
                            "removing from the domain: %ld",
                            policyId, it->use_count());
                        policies.erase(it);
                        return;
                    }
                }
            });

        policy->initialize();
        if (!force)
        {
            policy->verifyPolicy(params);
        }
        policy->updateParams(params);
        if (force)
        {
            policy->adjustCorrectableParameters();
            policy->validateParameters();
        }
        policy->postCreate();
        policy->setParentRunning(isRunning());
        policies.push_back(policy);
        return policy;
    }

    std::shared_ptr<DomainCapabilitiesIf> getDomainCapabilities() const
    {
        return domainInfo->capabilities;
    }

  protected:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::shared_ptr<TriggersManagerIf> triggersManager;
    std::shared_ptr<PolicyFactoryIf> policyFactory;
    std::shared_ptr<DomainInfo> domainInfo;
    DbusInterfaces dbusInterfaces{domainInfo->objectPath, objectServer};
    std::vector<std::shared_ptr<PolicyIf>> policies;

  private:
    virtual void createStatistics() = 0;

    virtual std::shared_ptr<PolicyIf>
        createPolicyFromFactory(PolicyId pId, const PolicyOwner policyOwner,
                                uint16_t statReportingPeriod, const bool force,
                                const DbusState dbusState,
                                PolicyEditable editable, bool allowDelete,
                                DeleteCallback deleteCallback) = 0;

    virtual int setCapabilitiesMax(const double& newValue, double& oldValue)
    {
        return -EPERM;
    }

    virtual double getCapabilitiesMax() const
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    virtual int setCapabilitiesMin(const double& newValue, double& oldValue)
    {
        return -EPERM;
    }

    virtual double getCapabilitiesMin() const
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    virtual uint32_t getCorrTimeMax() const
    {
        return std::numeric_limits<uint32_t>::quiet_NaN();
    }

    virtual uint32_t getCorrTimeMin() const
    {
        return std::numeric_limits<uint32_t>::quiet_NaN();
    }

    uint16_t getStatReportPeriodMax() const
    {
        return domainInfo->capabilities->getMaxStatReportingPeriod();
    }

    uint16_t getStatReportPeriodMin() const
    {
        return domainInfo->capabilities->getMinStatReportingPeriod();
    }

    virtual std::map<std::string, CapabilitiesValuesMap>
        getAllLimitsCapabilities() const = 0;

    std::string getDomainId() const
    {
        return enumToStr(kDomainIdNames, domainInfo->domainId);
    }

    std::vector<std::string> getAvailableTriggers() const
    {
        std::vector<std::string> names;
        for (auto trigger : *(domainInfo->triggers))
        {
            if (triggersManager->isTriggerAvailable(trigger))
            {
                names.push_back(enumToStr(kTriggerTypeNames, trigger));
            }
        }
        return names;
    }

    virtual int setLimitBiasAbsolute(const double& newValue, double& oldValue)
    {
        return -EPERM;
    }

    virtual double getLimitBiasAbsolute() const
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    virtual int setLimitBiasRelative(const double& newValue, double& oldValue)
    {
        return -EPERM;
    }

    virtual double getLimitBiasRelative() const
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    std::vector<DeviceIndex> getAvailableComponents() const
    {
        return *(domainInfo->availableComponents);
    }

    virtual sdbusplus::message::object_path
        createPolicyWithId(PolicyId policyIdArg, PolicyParamsTuple t)
    {
        throw errors::OperationNotPermitted();
    }

    virtual sdbusplus::message::object_path
        createPolicyForTotalBudget(PolicyId policyIdArg, PolicyParamsTuple t)
    {
        throw errors::OperationNotPermitted();
    }

    virtual sdbusplus::message::object_path getSelectedPolicyId() const
    {
        throw errors::OperationNotPermitted();
    }

    /**
     * @brief Returns policies paths for all policies with Selected state. If
     * there is a policy working for the whole domain, it is in the first
     * position in returned deque - before any per-component policies.
     *
     * @return std::deque<sdbusplus::message::object_path>
     */
    virtual std::deque<sdbusplus::message::object_path>
        getSelectedPolicies() const
    {
        std::deque<sdbusplus::message::object_path> policiesPaths;
        for (const auto& policy : policies)
        {
            if (policy->getState() == PolicyState::selected)
            {
                const auto& policyPath =
                    sdbusplus::message::object_path{policy->getObjectPath()};
                if (policy->getComponentId() == kComponentIdAll)
                {
                    policiesPaths.push_front(policyPath);
                }
                else
                {
                    policiesPaths.push_back(policyPath);
                }
            }
        }
        return policiesPaths;
    }

    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.Capabilities",
            [this](auto& capabilitiesIfce) {
                capabilitiesIfce.register_property_rw(
                    "Max", double(), sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        verifyRange<errors::OutOfRange>(
                            newValue, kMinSupportedCapabilityValue,
                            kMaxSupportedCapabilityValue);
                        return setCapabilitiesMax(newValue, oldValue);
                    },
                    [this](const auto&) { return getCapabilitiesMax(); });

                capabilitiesIfce.register_property_rw(
                    "Min", double(), sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        verifyRange<errors::OutOfRange>(
                            newValue, kMinSupportedCapabilityValue,
                            kMaxSupportedCapabilityValue);
                        return setCapabilitiesMin(newValue, oldValue);
                    },
                    [this](const auto&) { return getCapabilitiesMin(); });

                capabilitiesIfce.register_property_r(
                    "MaxCorrectionTimeInMs", uint32_t(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getCorrTimeMax(); });

                capabilitiesIfce.register_property_r(
                    "MinCorrectionTimeInMs", uint32_t(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getCorrTimeMin(); });

                capabilitiesIfce.register_property_r(
                    "MaxStatisticsReportingPeriod", uint16_t(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getStatReportPeriodMax(); });

                capabilitiesIfce.register_property_r(
                    "MinStatisticsReportingPeriod", uint16_t(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getStatReportPeriodMin(); });

                capabilitiesIfce.register_method(
                    "GetAllLimitsCapabilities",
                    [this]() { return getAllLimitsCapabilities(); });
            });

        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.DomainAttributes",
            [this](auto& domainAttributesIfce) {
                domainAttributesIfce.register_property_r(
                    "DomainId", std::string(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getDomainId(); });

                domainAttributesIfce.register_property_r(
                    "AvailableTriggers", std::vector<std::string>(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getAvailableTriggers(); });

                domainAttributesIfce.register_property_rw(
                    "LimitBiasAbsolute", double(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        return setLimitBiasAbsolute(newValue, oldValue);
                    },
                    [this](const auto&) { return getLimitBiasAbsolute(); });

                domainAttributesIfce.register_property_rw(
                    "LimitBiasRelative", double(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        return setLimitBiasRelative(newValue, oldValue);
                    },
                    [this](const auto&) { return getLimitBiasRelative(); });
                domainAttributesIfce.register_property_r(
                    "AvailableComponents", std::vector<DeviceIndex>(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return getAvailableComponents(); });
            });

        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.PolicyManager",
            [this](auto& iface) {
                iface.register_method(
                    "CreateWithId",
                    [this](PolicyId policyIdArg, PolicyParamsTuple t) {
                        return createPolicyWithId(policyIdArg, t);
                    });

                iface.register_method(
                    "CreateForTotalBudget",
                    [this](PolicyId policyIdArg, PolicyParamsTuple t) {
                        return createPolicyForTotalBudget(policyIdArg, t);
                    });

                iface.register_method("GetSelectedPolicyId", [this]() {
                    return getSelectedPolicyId();
                });

                iface.register_method("GetSelectedPolicies", [this]() {
                    return getSelectedPolicies();
                });
            });
    }
}; // class Domain

} // namespace nodemanager
