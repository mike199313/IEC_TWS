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

#include "additional_policy_dbus_properties.hpp"
#include "limit_exception_handler.hpp"
#include "limit_exception_monitor.hpp"
#include "policy.hpp"
#include "policy_storage_management.hpp"
#include "statistics/policy_accumulator.hpp"

namespace nodemanager
{

class PowerPolicy : public Policy, public AdditionalPolicyDbusProperties
{
  public:
    PowerPolicy() = delete;
    PowerPolicy(const PowerPolicy&) = delete;
    PowerPolicy& operator=(const PowerPolicy&) = delete;
    PowerPolicy(PowerPolicy&&) = delete;
    PowerPolicy& operator=(PowerPolicy&&) = delete;

    PowerPolicy(PolicyId policyIdArg, PolicyOwner ownerArg,
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
                const std::vector<std::shared_ptr<ComponentCapabilitiesIf>>&
                    componentCapabilitiesVectorArg) :
        Policy(policyIdArg, ownerArg, devicesManagerArg, gpioProviderArg,
               triggersManagerArg, storageManagementArg, statReportingPeriodArg,
               busArg, objectServerArg, domainInfoArg, deleteCallbackArg,
               dbusState, editableArg, allowDeleteArg),
        componentCapabilitiesVector(componentCapabilitiesVectorArg)
    {
        setVerifyDbusSetFunctions();
        setPostDbusSetFunctions();
        initializeDbusInterfaces();
        policyType.set(std::string("PowerPolicy"));
    }
    virtual ~PowerPolicy()
    {
    }

    void initialize() override
    {
        Policy::initialize();
        createLimitExceptionMonitor();
    }

    void setTresholdId(/*TresholdList*/)
    {
        // TODO: WIP
    }
    void setSuspendPeriod(/*SuspendPeriodList*/)
    {
        // TODO: WIP
    }
    bool isSuspended() const
    {
        return isSuspendedFlag;
    }

    virtual BudgetingStrategy getStrategy() const override
    {
        if (domainInfo->domainId == DomainId::MemorySubsystem)
        {
            return BudgetingStrategy::nonAggressive;
        }
        if (domainInfo->domainId == DomainId::HwProtection)
        {
            return BudgetingStrategy::immediate;
        }
        switch (powerCorrectionType.get())
        {
            case PowerCorrectionType::automatic:
                if (limitException.get() == LimitException::powerOff ||
                    limitException.get() == LimitException::logEventAndPowerOff)
                {
                    return BudgetingStrategy::aggressive;
                }
                else
                {
                    return BudgetingStrategy::nonAggressive;
                }
            case PowerCorrectionType::nonAggressive:
                return BudgetingStrategy::nonAggressive;
            case PowerCorrectionType::aggressive:
                return BudgetingStrategy::aggressive;
            default:
                throw std::logic_error(
                    "PowerCorrectionType has unsupported value " +
                    std::to_string(static_cast<
                                   std::underlying_type_t<PowerCorrectionType>>(
                        powerCorrectionType.get())));
        }
    }

    virtual void verifyParameters(const PolicyParams& params) const override
    {
        Policy::verifyParameters(params);
        verifyPowerCorrectionType(params.powerCorrectionType);
        verifyLimitException(params.limitException);
        verifyCorrectionTime(params.correctionInMs);

        // TODO verify if we are able to modify parameters when policy is
        // enabled else throw errors::PoliciesCannotBeCreated
    }

    virtual void updateParams(const PolicyParams& params) override
    {
        if (componentId.get() != params.componentId ||
            statReportingPeriod.get() != params.statReportingPeriod)
        {
            removeAllStatistics();

            addStatistics(
                std::make_shared<Statistic>(
                    kStatPolicyPower,
                    std::make_shared<PolicyAccumulator>(DurationMs{
                        std::chrono::seconds{params.statReportingPeriod}})),
                domainInfo->controlledParameter, params.componentId);
        }

        Policy::updateParams(params);

        // update all parameters exposed by this class
        correctionTime.set(params.correctionInMs);
        powerCorrectionType.set(params.powerCorrectionType);
        limitException.set(params.limitException);

        createLimitExceptionMonitor();
    }

    virtual void adjustCorrectableParameters() override
    {
        Policy::adjustCorrectableParameters();
        if ((correctionTime.get() <
             domainInfo->capabilities->getMinCorrectionTimeInMs()) ||
            (correctionTime.get() >
             domainInfo->capabilities->getMaxCorrectionTimeInMs()))
        {
            RedfishLogger::logPolicyAttributeAdjusted(getShortObjectPath());
            Logger::log<LogLevel::info>(
                "CorrectionInMs parameter for policy %s adjusted", getId());
            correctionTime.set(std::clamp(
                correctionTime.get(),
                domainInfo->capabilities->getMinCorrectionTimeInMs(),
                domainInfo->capabilities->getMaxCorrectionTimeInMs()));
        }
    }

    void run() override final
    {
        limitExceptionMonitor->run();
    }

  private:
    static constexpr const char* const kStatPolicyPower{"Power"}; //[Watts]

    const std::vector<std::shared_ptr<ComponentCapabilitiesIf>>&
        componentCapabilitiesVector;
    // suffix Flag is needed so the getter method is not named the same as
    // member
    bool isSuspendedFlag;
    // TODO: TresholdList
    // TODO: SuspendPeriodList
    std::unique_ptr<LimitExceptionMonitor> limitExceptionMonitor;

    std::shared_ptr<ComponentCapabilitiesIf>
        getComponentCapabilities(DeviceIndex idx) const
    {
        return idx < componentCapabilitiesVector.size()
                   ? componentCapabilitiesVector[idx]
                   : nullptr;
    }

    void verifyLimit(uint16_t limitArg, DeviceIndex componentIdArg,
                     TriggerType triggerTypeArg) const final override
    {
        if (triggerTypeArg == TriggerType::missingReadingsTimeout)
        {
            static constexpr const uint16_t min = 0;
            static constexpr const uint16_t max = 100;

            verifyRange<errors::PowerLimitOutOfRange>(limitArg, min, max);
        }
        else if (limitArg != kForceHighestThrottlingLevel)
        {
            auto componentCapabilities =
                getComponentCapabilities(componentIdArg);
            auto limitMin = componentCapabilities != nullptr
                                ? componentCapabilities->getMin()
                                : domainInfo->capabilities->getMin();
            verifyRange<errors::PowerLimitOutOfRange>(
                static_cast<double>(limitArg), limitMin, kMaxPowerLimitWatts);
        }
    }

    void verifyCorrectionTime(uint32_t correctionInMsArg) const
    {
        verifyRange<errors::CorrectionTimeOutOfRange>(
            correctionInMsArg,
            domainInfo->capabilities->getMinCorrectionTimeInMs(),
            domainInfo->capabilities->getMaxCorrectionTimeInMs());
    }

    void verifyPowerCorrectionType(
        const PowerCorrectionType& powerCorrectionTypeArg) const
    {
        toEnum<errors::InvalidPowerCorrectionType>(kPowerCorrectionType,
                                                   powerCorrectionTypeArg);
    }

    void verifyLimitException(const LimitException& limitExceptionArg) const
    {
        toEnum<errors::InvalidLimitException>(kLimitException,
                                              limitExceptionArg);
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
        limit.setPostDbusSetFunction([this]() {
            postParameterUpdate(false);
            limitExceptionMonitor->reset();
        });

        correctionTime.setPostDbusSetFunction([this]() {
            postParameterUpdate(false);
            limitExceptionMonitor->reset();
        });

        powerCorrectionType.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));

        limitException.setPostDbusSetFunction([this]() {
            postParameterUpdate(false);
            limitExceptionMonitor->reset();
        });
    }

    void createLimitExceptionMonitor()
    {
        limitExceptionMonitor = std::make_unique<LimitExceptionMonitor>(
            shared_from_this(),
            std::make_shared<LimitExceptionHandler>(
                getShortObjectPath(), kLimitExceptionDbusConfig, bus),
            devicesManager, domainInfo->controlledParameter, limit,
            correctionTime, limitException);
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

    void getPolicyParams(PolicyParams& params) const override
    {
        Policy::getPolicyParams(params);
        params.correctionInMs = correctionTime.get();
        params.powerCorrectionType = powerCorrectionType.get();
        params.limitException = limitException.get();
        params.suspendPeriods = PolicySuspendPeriods();
        params.thresholds = PolicyThresholds();
    }
};

} // namespace nodemanager
