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

#include "common_types.hpp"
#include "domain.hpp"
#include "efficiency_control.hpp"
#include "loggers/log.hpp"
#include "policies/performance_policy.hpp"
#include "policies/policy_factory.hpp"
#include "policies/policy_types.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/enum_to_string.hpp"

#include <iostream>

#pragma once

namespace nodemanager
{

static const std::shared_ptr<std::vector<TriggerType>>
    kTriggersInDomainPerformance =
        std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
            TriggerType::always, TriggerType::inletTemperature,
            TriggerType::gpio});
class DomainPerformance : public Domain
{
  public:
    DomainPerformance() = delete;
    DomainPerformance(const DomainPerformance&) = delete;
    DomainPerformance& operator=(const DomainPerformance&) = delete;
    DomainPerformance(DomainPerformance&&) = delete;
    DomainPerformance& operator=(DomainPerformance&&) = delete;

    DomainPerformance(
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<sdbusplus::asio::object_server>& objectServerArg,
        std::string const& objectPathArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        const std::shared_ptr<GpioProviderIf>& gpioProviderArg,
        const std::shared_ptr<TriggersManagerIf>& triggersManagerArg,
        const std::shared_ptr<EfficiencyControlIf>& efficiencyControlArg,
        const std::shared_ptr<PolicyFactoryIf>& policyFactoryArg,
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactoryArg,
        const DbusState dbusState) :
        Domain(busArg, objectServerArg, objectPathArg, devicesManagerArg,
               gpioProviderArg, triggersManagerArg, ID, policyFactoryArg,
               createDomainInfo(objectPathArg, capabilitiesFactoryArg),
               dbusState),
        efficiencyControl(efficiencyControlArg)
    {
        registerCapabilities(capabilitiesFactoryArg);
        createStatistics();
    }

    virtual ~DomainPerformance() = default;

    void run() override final
    {
        Domain::run();
        processPoliciesKnobValues();
    }

    void createDefaultPolicies() override final
    {
        for (auto const& knobCapabilities : knobCapabilitiesVector)
        {
            auto knobType = knobCapabilities->getKnobType();

            if (!findPolicy(knobType))
            {
                Domain::createPolicy(
                    enumToStr(knobTypeNames, knobType), PolicyOwner::bmc,
                    PolicyParams{
                        kCorrectionTimePerformancePolicy,
                        std::numeric_limits<uint16_t>::quiet_NaN(),
                        domainInfo->capabilities->getMinStatReportingPeriod(),
                        PolicyStorage::volatileStorage,
                        PowerCorrectionType::automatic,
                        LimitException::noAction, PolicySuspendPeriods{},
                        PolicyThresholds{}, uint8_t{kAllDevices},
                        kZeroTriggerLimit, "AlwaysOn"},
                    kForcedCreate, DbusState::disabled, PolicyEditable::yes,
                    false);
            }
        }
    }

    static constexpr DomainId ID = DomainId::Performance;

  private:
    std::shared_ptr<EfficiencyControlIf> efficiencyControl;
    std::vector<std::shared_ptr<KnobCapabilitiesIf>> knobCapabilitiesVector;

    static constexpr auto kStatCpuFrequency{"Frequency"};
    static constexpr double kPerfKnobCapMin{0.0};
    static constexpr double kHwpmPerfBiasMax{0xf};
    static constexpr double kHwpmPerfPreferenceOverrideMax{0x7};

    std::shared_ptr<PolicyIf>
        createPolicyFromFactory(PolicyId pId, const PolicyOwner policyOwner,
                                uint16_t statReportingPeriod, const bool force,
                                const DbusState dbusState,
                                PolicyEditable editable, bool allowDelete,
                                DeleteCallback deleteCallback) override final
    {
        return policyFactory->createPolicy(
            domainInfo, PolicyType::performance, pId, policyOwner,
            statReportingPeriod, deleteCallback, dbusState, editable,
            allowDelete, getKnobCapabilities(pId),
            std::vector<std::shared_ptr<ComponentCapabilitiesIf>>{});
    }

    std::shared_ptr<KnobCapabilitiesIf>
        getKnobCapabilities(PolicyId policyId) const
    {
        KnobType knobType{
            strToEnum<errors::InvalidPolicyId>(knobTypeNames, policyId)};

        for (const auto& caps : knobCapabilitiesVector)
        {
            if (knobType == caps->getKnobType())
            {
                return caps;
            }
        }

        return nullptr;
    }

    void onKnobCapabilitiesChange(KnobType knobType)
    {
        for (const auto& policy : policies)
        {
            if (std::static_pointer_cast<PerformancePolicy>(policy)
                    ->getKnobType() == knobType)
            {
                policy->validateParameters();
            }
        }
    }

    void registerCapabilities(
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactory)
    {
        knobCapabilitiesVector.push_back(
            capabilitiesFactory->createKnobCapabilities(
                ReadingType::turboRatioCapabilitiesMin,
                ReadingType::turboRatioCapabilitiesMax,
                KnobType::TurboRatioLimit,
                std::bind(&DomainPerformance::onKnobCapabilitiesChange, this,
                          KnobType::TurboRatioLimit)));

        knobCapabilitiesVector.push_back(
            capabilitiesFactory->createKnobCapabilities(
                ReadingType::prochotRatioCapabilitiesMin,
                ReadingType::prochotRatioCapabilitiesMax, KnobType::Prochot,
                std::bind(&DomainPerformance::onKnobCapabilitiesChange, this,
                          KnobType::Prochot)));

        knobCapabilitiesVector.push_back(
            capabilitiesFactory->createKnobCapabilities(
                std::numeric_limits<double>::quiet_NaN(),
                std::numeric_limits<double>::quiet_NaN(),
                KnobType::HwpmPerfPreference));

        knobCapabilitiesVector.push_back(
            capabilitiesFactory->createKnobCapabilities(
                kPerfKnobCapMin, kHwpmPerfBiasMax, KnobType::HwpmPerfBias));

        knobCapabilitiesVector.push_back(
            capabilitiesFactory->createKnobCapabilities(
                kPerfKnobCapMin, kHwpmPerfPreferenceOverrideMax,
                KnobType::HwpmPerfPreferenceOverride));
    }

    void createStatistics() override final
    {
        addStatistics(
            std::make_shared<Statistic>(std::string(kStatCpuFrequency),
                                        std::make_shared<GlobalAccumulator>()),
            domainInfo->controlledParameter);
    }

    void processPoliciesKnobValues()
    {
        for (auto& policy : policies)
        {
            auto policyState = policy->getState();

            if (PolicyState::triggered == policyState)
            {
                policy->setLimitSelected(true);
            }

            if (PolicyState::selected == policyState)
            {
                efficiencyControl->setValue(
                    std::static_pointer_cast<PerformancePolicy>(policy)
                        ->getKnobType(),
                    policy->getLimit());
            }
            else
            {
                efficiencyControl->resetValue(
                    std::static_pointer_cast<PerformancePolicy>(policy)
                        ->getKnobType());
            }
        }
    }

    bool findPolicy(KnobType knobType)
    {
        for (auto const& policy : policies)
        {
            if (std::static_pointer_cast<PerformancePolicy>(policy)
                    ->getKnobType() == knobType)
            {
                return true;
            }
        }

        return false;
    }

    static std::shared_ptr<DomainInfo> createDomainInfo(
        std::string const& objectPath,
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactory)
    {
        return std::make_shared<DomainInfo>(
            DomainInfo{objectPath + "/Domain/" + enumToStr(kDomainIdNames, ID),
                       ReadingType::cpuAverageFrequency,
                       capabilitiesFactory->createDomainCapabilities(
                           std::nullopt, std::nullopt,
                           kCorrectionTimePerformancePolicy, nullptr, ID),
                       ID, std::make_shared<std::vector<DeviceIndex>>(), false,
                       kTriggersInDomainPerformance, kComponentIdIgnored});
    }

    std::map<std::string, CapabilitiesValuesMap>
        getAllLimitsCapabilities() const override
    {
        std::map<std::string, CapabilitiesValuesMap> retMap;
        for (auto const& knobCapabilities : knobCapabilitiesVector)
        {
            retMap.emplace(knobCapabilities->getName(),
                           knobCapabilities->getValuesMap());
        }
        return retMap;
    }

    uint32_t getCorrTimeMax() const override
    {
        return kCorrectionTimePerformancePolicy;
    }

    uint32_t getCorrTimeMin() const override
    {
        return kCorrectionTimePerformancePolicy;
    }

}; // class DomainPerformance

} // namespace nodemanager
