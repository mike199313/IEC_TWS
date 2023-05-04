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

#include "additional_policy_dbus_properties.hpp"
#include "common_types.hpp"
#include "domains/capabilities/knob_capabilities.hpp"
#include "knobs/knob.hpp"
#include "limit_exception_monitor.hpp"
#include "policy.hpp"
#include "policy_types.hpp"

#include <iostream>

namespace nodemanager
{

static constexpr uint32_t kCorrectionTimePerformancePolicy =
    std::numeric_limits<uint32_t>::quiet_NaN();
class PerformancePolicy : public Policy, public AdditionalPolicyDbusProperties
{
  public:
    PerformancePolicy() = delete;
    PerformancePolicy(const PerformancePolicy&) = delete;
    PerformancePolicy& operator=(const PerformancePolicy&) = delete;
    PerformancePolicy(PerformancePolicy&&) = delete;
    PerformancePolicy& operator=(PerformancePolicy&&) = delete;

    PerformancePolicy(
        PolicyId policyIdArg, PolicyOwner ownerArg,
        std::shared_ptr<DevicesManagerIf> devicesManagerArg,
        std::shared_ptr<GpioProviderIf> gpioProviderArg,
        std::shared_ptr<TriggersManagerIf> triggersManagerArg,
        std::shared_ptr<PolicyStorageManagementIf> storageManagementArg,
        uint16_t statReportingPeriodArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg,
        std::shared_ptr<sdbusplus::asio::object_server> objectServerArg,
        std::shared_ptr<DomainInfo>& domainInfoArg,
        const DeleteCallback deleteCallbackArg, DbusState dbusState,
        PolicyEditable editableArg, bool allowDeleteArg,
        std::shared_ptr<KnobCapabilitiesIf> knobCapabilitiesArg) :
        Policy(policyIdArg, ownerArg, devicesManagerArg, gpioProviderArg,
               triggersManagerArg, storageManagementArg, statReportingPeriodArg,
               busArg, objectServerArg, domainInfoArg, deleteCallbackArg,
               dbusState, editableArg, allowDeleteArg),
        knobCapabilities(knobCapabilitiesArg)
    {
        setVerifyDbusSetFunctions();
        setPostDbusSetFunctions();
        initializeDbusInterfaces();
        policyType.set(std::string("PerformancePolicy"));
        owner.set(toEnum<errors::PoliciesCannotBeCreated>(
            std::set<PolicyOwner>{PolicyOwner::bmc}, ownerArg));
    }

    virtual ~PerformancePolicy() = default;

    virtual BudgetingStrategy getStrategy() const override
    {
        throw std::logic_error(
            "Trying to call getStrategy() for performance policy");
    }

    void verifyParameters(const PolicyParams& params) const final override
    {
        Policy::verifyParameters(params);
        verifyPowerCorrectionType(params.powerCorrectionType);
        verifyLimitException(params.limitException);
        verifyCorrectionTime(params.correctionInMs);
        verifyKnobType();
    }

    void updateParams(const PolicyParams& params) final override
    {
        if (statReportingPeriod.get() != params.statReportingPeriod)
        {
            removeAllStatistics();

            addStatistics(
                std::make_shared<Statistic>(
                    kStatPolicyFrequency,
                    std::make_shared<PolicyAccumulator>(DurationMs{
                        std::chrono::seconds{params.statReportingPeriod}})),
                domainInfo->controlledParameter, params.componentId);
        }

        Policy::updateParams(params);

        correctionTime.set(params.correctionInMs);
        powerCorrectionType.set(params.powerCorrectionType);
        limitException.set(params.limitException);
    }

    KnobType getKnobType() const
    {
        return knobCapabilities->getKnobType();
    }

  private:
    static constexpr auto kStatPolicyFrequency = "Frequency";

    std::shared_ptr<KnobCapabilitiesIf> knobCapabilities;
    std::unique_ptr<LimitExceptionMonitor> limitExceptionMonitor;

    void verifyLimit(
        uint16_t limitArg, [[maybe_unused]] DeviceIndex componentIdArg,
        [[maybe_unused]] TriggerType triggerTypeArg) const final override
    {
        double min{knobCapabilities->getMin()};
        double max{knobCapabilities->getMax()};

        if (!std::isnan(min) && !std::isnan(max))
        {
            verifyRange<errors::PowerLimitOutOfRange>(
                static_cast<double>(limitArg), min, max);
        }
    }

    void verifyPowerCorrectionType(
        const PowerCorrectionType& powerCorrectionTypeArg) const
    {
        if (powerCorrectionTypeArg != PowerCorrectionType::automatic)
        {
            throw errors::InvalidPowerCorrectionType();
        }
    }

    void verifyLimitException(const LimitException& limitExceptionArg) const
    {
        if (limitExceptionArg != LimitException::noAction)
        {
            throw errors::InvalidLimitException();
        }
    }

    void verifyCorrectionTime(uint32_t correctionInMsArg) const
    {
        if (correctionInMsArg != kCorrectionTimePerformancePolicy)
        {
            throw errors::CorrectionTimeOutOfRange();
        }
    }

    void getPolicyParams(PolicyParams& params) const final override
    {
        Policy::getPolicyParams(params);
        params.correctionInMs = correctionTime.get();
        params.powerCorrectionType = powerCorrectionType.get();
        params.limitException = limitException.get();
        params.suspendPeriods = PolicySuspendPeriods();
        params.thresholds = PolicyThresholds();
    }

    void verifyKnobType() const
    {
        KnobType knob{getKnobType()};

        switch (knob)
        {
            case KnobType::TurboRatioLimit:
            case KnobType::Prochot:
            case KnobType::HwpmPerfPreference:
            case KnobType::HwpmPerfBias:
            case KnobType::HwpmPerfPreferenceOverride:
                break;
            default:
                throw std::logic_error(
                    "Invalid knob type, expected efficiency knob while we "
                    "have: " +
                    std::to_string(
                        static_cast<std::underlying_type_t<KnobType>>(knob)));
        }
    }

    void setVerifyDbusSetFunctions()
    {
        correctionTime.setVerifyDbusSetFunction(
            [this](uint32_t correctionTimeArg) {
                this->verifyCorrectionTime(correctionTimeArg);
            });

        powerCorrectionType.setVerifyDbusSetFunction(
            [this](PowerCorrectionType powerCorrectionTypeArg) {
                this->verifyPowerCorrectionType(powerCorrectionTypeArg);
            });

        limitException.setVerifyDbusSetFunction(
            [this](LimitException limitExceptionArg) {
                this->verifyLimitException(limitExceptionArg);
            });
    }

    void setPostDbusSetFunctions()
    {
        correctionTime.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));

        powerCorrectionType.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));

        limitException.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));
    }

    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.PolicyAttributes",
            [this](auto& policyAttributesInterface) {
                AdditionalPolicyDbusProperties::registerProperties(
                    policyAttributesInterface, isEditable());
            });
    }
}; // class PerformancePolicy

} // namespace nodemanager
