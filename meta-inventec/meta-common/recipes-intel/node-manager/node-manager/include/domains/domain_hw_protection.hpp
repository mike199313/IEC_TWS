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

#include "config/config.hpp"
#include "domain_power.hpp"

namespace nodemanager
{

static constexpr const auto kHwProtectionPowerAlwaysOnPolicyId =
    "HwProtectionAlwaysOn";
static constexpr const auto kHwProtectionPowerGpioPolicyId = "HwProtectionGpio";
static constexpr const uint32_t kHwProtectionPolicyCorrectionTime = 1000;
static constexpr const double kDefaultPsuMinPowerCapability = 0;

static const std::shared_ptr<std::vector<TriggerType>>
    kTriggersInDomainHwProtection = std::make_shared<std::vector<TriggerType>>(
        std::vector<TriggerType>{TriggerType::always, TriggerType::gpio});

class DomainHwProtection : public DomainPower
{
  public:
    DomainHwProtection() = delete;
    DomainHwProtection(const DomainHwProtection&) = delete;
    DomainHwProtection& operator=(const DomainHwProtection&) = delete;
    DomainHwProtection(DomainHwProtection&&) = delete;
    DomainHwProtection& operator=(DomainHwProtection&&) = delete;

    DomainHwProtection(
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServerArg,
        std::string const& objectPathArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        const std::shared_ptr<GpioProviderIf>& gpioProviderArg,
        const std::shared_ptr<TriggersManagerIf>& triggersManagerArg,
        const std::shared_ptr<BudgetingIf>& budgetingArg,
        const std::shared_ptr<PolicyFactoryIf>& policyFactoryArg,
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactory,
        const DbusState dbusState) :
        DomainPower(busArg, objServerArg, objectPathArg, devicesManagerArg,
                    gpioProviderArg, triggersManagerArg, budgetingArg, ID,
                    ReadingType::hwProtectionPlatformPower, std::nullopt,
                    ReadingType::hwProtectionPowerCapabilitiesMax,
                    ReadingType::hwProtectionPlatformPower,
                    ReadingType::dcPlatformEnergy, policyFactoryArg,
                    capabilitiesFactory, kComponentIdIgnored,
                    kTriggersInDomainHwProtection, kPolicyMinCorrectionTimeInMs,
                    dbusState)
    {
        registerLimitReadingPsu();
        registerReadingEventCallbacks();
    }

    virtual ~DomainHwProtection() = default;

    void createDefaultPolicies() override
    {
        createHwProtectionPolicies();
    }

    static constexpr DomainId ID = DomainId::HwProtection;

  protected:
    virtual void onCapabilitiesChange() override
    {
        DomainPower::onCapabilitiesChange();
        if (getReadingSource() != SensorReadingType::dcPlatformPowerPsu)
        {
            updateLimit(
                static_cast<uint16_t>(domainInfo->capabilities->getMax()));
        }
    }

  private:
    std::shared_ptr<PolicyIf>
        createPolicyFromFactory(PolicyId pId, const PolicyOwner policyOwner,
                                uint16_t statReportingPeriod, const bool force,
                                const DbusState dbusState,
                                PolicyEditable editable, bool allowDelete,
                                DeleteCallback deleteCallback) override final
    {
        if (policyOwner != PolicyOwner::internal)
        {
            throw errors::OperationNotPermitted();
        }
        return DomainPower::createPolicyFromFactory(
            pId, policyOwner, statReportingPeriod, force, dbusState, editable,
            allowDelete, deleteCallback);
    }

    void createHwProtectionPolicies()
    {
        createPolicy(
            kHwProtectionPowerAlwaysOnPolicyId, PolicyOwner::internal,
            PolicyParams{
                kHwProtectionPolicyCorrectionTime,
                static_cast<uint16_t>(domainInfo->capabilities->getMax()),
                kMinimumStatReportingPeriod, PolicyStorage::volatileStorage,
                PowerCorrectionType::automatic, LimitException::noAction,
                PolicySuspendPeriods{}, PolicyThresholds{},
                uint8_t{kAllDevices}, kZeroTriggerLimit, "AlwaysOn"},
            kForcedCreate, DbusState::alwaysEnabled, PolicyEditable::no, false);

        std::string hwProtectionGpioName =
            Config::getInstance().getGpio().hwProtectionPolicyTriggerGpio;
        std::optional<DeviceIndex> gpioLine =
            gpioProvider->getGpioLine(hwProtectionGpioName);

        if (gpioLine)
        {
            createPolicy(
                kHwProtectionPowerGpioPolicyId, PolicyOwner::internal,
                PolicyParams{
                    kHwProtectionPolicyCorrectionTime,
                    static_cast<uint16_t>(domainInfo->capabilities->getMax()),
                    kMinimumStatReportingPeriod, PolicyStorage::volatileStorage,
                    PowerCorrectionType::automatic, LimitException::noAction,
                    PolicySuspendPeriods{}, PolicyThresholds{},
                    uint8_t{kAllDevices}, *gpioLine, "GPIO"},
                kForcedCreate, DbusState::alwaysEnabled, PolicyEditable::no,
                false);
        }
    }

    void registerLimitReadingPsu()
    {
        devicesManager->registerReadingConsumer(
            std::make_shared<ReadingEvent>([this](double reading) {
                if (!std::isnan(reading) &&
                    (getReadingSource() ==
                     SensorReadingType::dcPlatformPowerPsu))
                {
                    updateLimit(static_cast<uint16_t>(reading));
                }
            }),
            ReadingType::dcRatedPowerMin);
    }

    void registerReadingEventCallbacks()
    {
        devicesManager->registerReadingConsumer(
            std::make_shared<ReadingEvent>(
                nullptr, nullptr,
                [this](const ReadingEventType eventType,
                       const ReadingContext& readingCtx) {
                    if (eventType == ReadingEventType::readingUnavailable)
                    {
                        Logger::log<LogLevel::error>(
                            "Capabilities max reading unavailable for HW "
                            "Protection domain");
                    }
                }),
            ReadingType::hwProtectionPowerCapabilitiesMax);

        devicesManager->registerReadingConsumer(
            std::make_shared<ReadingEvent>(
                nullptr, nullptr,
                [this](const ReadingEventType eventType,
                       const ReadingContext& readingCtx) {
                    if (eventType == ReadingEventType::readingSourceChanged)
                    {
                        if (getReadingSource() ==
                            SensorReadingType::dcPlatformPowerPsu)
                        {
                            domainInfo->capabilities->setMax(
                                domainInfo->capabilities->getMaxRated());
                            domainInfo->capabilities->setMin(
                                kDefaultPsuMinPowerCapability);
                        }
                    }
                }),
            ReadingType::hwProtectionPlatformPower);
    }

    void updateLimit(uint16_t limit)
    {
        for (std::shared_ptr<nodemanager::PolicyIf> policy : policies)
        {
            policy->setLimit(limit);
        }
    }

    std::optional<SensorReadingType> getReadingSource()
    {
        const auto& reading =
            devicesManager->findReading(domainInfo->controlledParameter);
        if (reading)
        {
            return reading->getReadingSource();
        }
        return std::nullopt;
    }

    int setCapabilitiesMax(const double& newValue, double& oldValue) override
    {
        if (getReadingSource() == SensorReadingType::dcPlatformPowerPsu)
        {
            throw errors::CmdNotSupported();
        }
        else if (domainInfo->capabilities->getMaxRated() < newValue)
        {
            throw errors::OutOfRange();
        }
        else
        {
            return DomainPower::setCapabilitiesMax(newValue, oldValue);
        }
    }

    int setCapabilitiesMin(const double& newValue, double& oldValue) override
    {
        if (getReadingSource() == SensorReadingType::dcPlatformPowerPsu)
        {
            throw errors::CmdNotSupported();
        }
        else
        {
            return DomainPower::setCapabilitiesMin(newValue, oldValue);
        }
    }
};

} // namespace nodemanager
