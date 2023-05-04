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

#include "budgeting/budgeting.hpp"
#include "capabilities/capabilities_factory.hpp"
#include "common_types.hpp"
#include "domain.hpp"
#include "domain_types.hpp"
#include "policies/policy_factory.hpp"
#include "policies/power_policy.hpp"
#include "readings/reading_event.hpp"
#include "scoped_resource.hpp"
#include "statistics/energy_statistic.hpp"
#include "statistics/statistic.hpp"
#include "statistics/statistics_provider.hpp"
#include "statistics/throttling_statistic.hpp"
#include "triggers/trigger_enums.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/dbus_interfaces.hpp"
#include "utility/enum_to_string.hpp"
#include "utility/property.hpp"
#include "utility/ranges.hpp"

#include <iostream>

namespace nodemanager
{

static constexpr const auto kStatGlobalPower = "Power";           //[Watts]
static constexpr const auto kStatGlobalThrottling = "Throttling"; //[%]
static constexpr const auto kStatGlobalEnergyAccum =
    "Energy accumulator"; // [J]
static constexpr const std::chrono::milliseconds kPolicyMinCorrectionTimeInMs{
    1000};
static constexpr const auto kDmtfPowerPolicyId = "DmtfPower";
static constexpr const auto kSmbalertPowerPolicyId = "SMBAlert";
static constexpr const uint32_t kInternalPolicyCorrectionTime = 1000;
static constexpr const uint32_t kInternalSmartPolicyCorrectionTime = 4000;
static constexpr const uint16_t kZeroWattLimit = 0;
static constexpr const uint16_t kMinimumStatReportingPeriod = 1;
static constexpr const uint16_t kDmtfStatReportingPeriod = 60;
static constexpr const double kMinBiasValue = 0.0;
static constexpr const double kMaxBiasValue = 16383.0;

using DomainLimits =
    std::unordered_map<std::pair<DeviceIndex, BudgetingStrategy>,
                       std::weak_ptr<PolicyIf>,
                       boost::hash<std::pair<DeviceIndex, BudgetingStrategy>>>;
class DomainPower : public Domain
{
  public:
    DomainPower() = delete;
    DomainPower(const DomainPower&) = delete;
    DomainPower& operator=(const DomainPower&) = delete;
    DomainPower(DomainPower&&) = delete;
    DomainPower& operator=(DomainPower&&) = delete;

    DomainPower(
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<sdbusplus::asio::object_server>& objectServerArg,
        std::string const& objectPathArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        const std::shared_ptr<GpioProviderIf>& gpioProviderArg,
        const std::shared_ptr<TriggersManagerIf>& triggersManagerArg,
        const std::shared_ptr<BudgetingIf>& budgetingArg, DomainId idArg,
        ReadingType controlledParameterArg,
        std::optional<ReadingType> minCapabilityReadingArg,
        std::optional<ReadingType> maxCapabilityReadingArg,
        std::optional<ReadingType> limitReadingTypeArg,
        std::optional<ReadingType> energyReadingTypeArg,
        const std::shared_ptr<PolicyFactoryIf>& policyFactoryArg,
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactory,
        DeviceIndex maxComponentNumberArg,
        const std::shared_ptr<std::vector<TriggerType>>& triggers,
        std::chrono::milliseconds policyCorrectionTime, DbusState dbusState) :
        Domain(busArg, objectServerArg, objectPathArg, devicesManagerArg,
               gpioProviderArg, triggersManagerArg, idArg, policyFactoryArg,
               createDomainInfo(objectPathArg, idArg, controlledParameterArg,
                                minCapabilityReadingArg,
                                maxCapabilityReadingArg, capabilitiesFactory,
                                maxComponentNumberArg, triggers,
                                policyCorrectionTime, idArg),
               dbusState),

        budgeting(budgetingArg), limitReadingType(limitReadingTypeArg),
        energyReadingType(energyReadingTypeArg)
    {
        registerCapabilities(capabilitiesFactory, minCapabilityReadingArg,
                             maxCapabilityReadingArg);

        hostPowerReadingEvent =
            std::make_shared<ReadingEvent>([this](double value) {
                auto tmp = static_cast<bool>(value);
                if (isHostPowerOn && !tmp)
                {
                    Logger::log<LogLevel::info>(
                        "HostPowerOff detected, suspending budget limiting for "
                        "domain:%d",
                        static_cast<int>(domainInfo->domainId));
                    deleteNonActiveLimits(DomainLimits{});
                }
                isHostPowerOn = tmp;
            });
        devicesManager->registerReadingConsumer(hostPowerReadingEvent,
                                                ReadingType::hostPower);

        createStatistics();
        registerRequiredReading(controlledParameterArg);
    }

    virtual ~DomainPower()
    {
        devicesManager->unregisterReadingConsumer(
            availableComponentsReadingEvent);
        devicesManager->unregisterReadingConsumer(requiredReadingEvent);
        devicesManager->unregisterReadingConsumer(hostPowerReadingEvent);
    }

    void run() override final
    {
        Domain::run();

        if (isHostPowerOn)
        {
            auto limits = getTriggeredPoliciesLimits();
            updateLimits(limits);
            deleteNonActiveLimits(limits);
            // TODO: Consider running it on create/update/delete policy events
            // to save some cycles
        }
    }

    void postRun() override final
    {
        Domain::postRun();

        if (isHostPowerOn)
        {
            matchPolicyWithSelectedLimit();
        }
    }

  protected:
    std::shared_ptr<BudgetingIf> budgeting;
    std::vector<std::shared_ptr<ComponentCapabilitiesIf>>
        componentCapabilitiesVector;
    DomainLimits limitingPolicies;
    bool biasUpdated;
    std::shared_ptr<ReadingEvent> hostPowerReadingEvent;
    bool isHostPowerOn = false;
    double prevAvailableComponents = 0;
    std::shared_ptr<ReadingEvent> availableComponentsReadingEvent;
    std::shared_ptr<ReadingEvent> requiredReadingEvent = std::make_shared<
        ReadingEvent>(
        nullptr, nullptr,
        [this](const ReadingEventType eventType,
               const ReadingContext& readingCtx) {
            switch (eventType)
            {
                case ReadingEventType::readingUnavailable:
                    domainInfo->requiredReadingUnavailable = true;
                    break;
                case ReadingEventType::readingAvailable:
                    domainInfo->requiredReadingUnavailable = false;
                    break;
                default:
                    Logger::log<LogLevel::debug>(
                        "Unexpected ReadingEvent type: %d for domain "
                        "required reading",
                        static_cast<std::underlying_type_t<ReadingEventType>>(
                            eventType));
                    return;
            }
            for (std::shared_ptr<nodemanager::PolicyIf> policy : policies)
            {
                policy->validateParameters();
            }
        });

    template <DeviceIndex S>
    void registerAvailableComponents(ReadingType readingType)
    {
        availableComponentsReadingEvent =
            std::make_shared<ReadingEvent>([this](double value) {
                if (value != prevAvailableComponents)
                {
                    domainInfo->availableComponents->clear();
                    if (!std::isnan(value))
                    {
                        std::bitset<S> devicePresenceMap =
                            static_cast<unsigned long>(value);
                        for (DeviceIndex idx = 0;
                             idx < devicePresenceMap.size(); idx++)
                        {
                            if (devicePresenceMap[idx])
                            {
                                domainInfo->availableComponents->push_back(idx);
                            }
                        }
                    }
                    validatePolicies();
                    prevAvailableComponents = value;
                }
            });
        devicesManager->registerReadingConsumer(availableComponentsReadingEvent,
                                                readingType);
    }

    void registerRequiredReading(ReadingType readingType)
    {
        domainInfo->requiredReadingUnavailable = true;
        devicesManager->registerReadingConsumer(requiredReadingEvent,
                                                readingType);
    }

    virtual void onCapabilitiesChange()
    {
        validatePolicies();
    }

    void createDmtfPolicies(std::string singularName, std::string pluralName)
    {
        createPolicy(kDmtfPowerPolicyId + pluralName, PolicyOwner::internal,
                     PolicyParams{kInternalPolicyCorrectionTime, kZeroWattLimit,
                                  kDmtfStatReportingPeriod,
                                  PolicyStorage::volatileStorage,
                                  PowerCorrectionType::nonAggressive,
                                  LimitException::noAction,
                                  PolicySuspendPeriods{}, PolicyThresholds{},
                                  kAllDevices, kZeroTriggerLimit, "AlwaysOn"},
                     kForcedCreate, DbusState::disabled, PolicyEditable::yes,
                     false);

        for (DeviceIndex idx = 0; idx < domainInfo->maxComponentNumber; idx++)
        {
            createPolicy(
                getDmtfPolicyId(idx, singularName), PolicyOwner::internal,
                PolicyParams{kInternalPolicyCorrectionTime, kZeroWattLimit,
                             kDmtfStatReportingPeriod,
                             PolicyStorage::volatileStorage,
                             PowerCorrectionType::nonAggressive,
                             LimitException::noAction, PolicySuspendPeriods{},
                             PolicyThresholds{}, uint8_t{idx},
                             kZeroTriggerLimit, "AlwaysOn"},
                kForcedCreate, DbusState::disabled, PolicyEditable::yes, false);
        }
    }

    virtual std::shared_ptr<PolicyIf>
        createPolicyFromFactory(PolicyId pId, const PolicyOwner policyOwner,
                                uint16_t statReportingPeriod, const bool force,
                                const DbusState dbusState,
                                PolicyEditable editable, bool allowDelete,
                                DeleteCallback deleteCallback) override
    {
        return policyFactory->createPolicy(
            domainInfo, PolicyType::power, pId, policyOwner,
            statReportingPeriod, deleteCallback, dbusState, editable,
            allowDelete, nullptr, componentCapabilitiesVector);
    }

    PolicyId getDmtfPolicyId(DeviceIndex deviceIdx, std::string name)
    {
        return kDmtfPowerPolicyId + name + std::to_string(deviceIdx);
    }

    virtual int setCapabilitiesMax(const double& newValue,
                                   double& oldValue) override
    {
        domainInfo->capabilities->setMax(oldValue = newValue);
        return 1;
    }

    virtual int setCapabilitiesMin(const double& newValue,
                                   double& oldValue) override
    {
        domainInfo->capabilities->setMin(oldValue = newValue);
        return 1;
    }

  private:
    double limitBiasAbsolute = 0.0;
    double limitBiasRelative = 1.0;
    std::optional<ReadingType> limitReadingType;
    std::optional<ReadingType> energyReadingType;

    void registerCapabilities(
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactory,
        const std::optional<ReadingType> minCapapbilityReadingType,
        const std::optional<ReadingType> maxCapapbilityReadingType)
    {
        for (DeviceIndex deviceIndex = 0;
             deviceIndex < domainInfo->maxComponentNumber; ++deviceIndex)
        {
            componentCapabilitiesVector.push_back(
                capabilitiesFactory->createComponentCapabilities(
                    minCapapbilityReadingType, maxCapapbilityReadingType,
                    deviceIndex));
        }
    }

    std::shared_ptr<ComponentCapabilitiesIf>
        getComponentCapabilities(DeviceIndex idx) const
    {
        return idx < componentCapabilitiesVector.size()
                   ? componentCapabilitiesVector[idx]
                   : nullptr;
    }

    void createStatisticsPower()
    {
        addStatistics(
            std::make_shared<Statistic>(kStatGlobalPower,
                                        std::make_shared<GlobalAccumulator>()),
            domainInfo->controlledParameter);

        for (DeviceIndex devIndex = 0;
             devIndex < domainInfo->maxComponentNumber; ++devIndex)
        {
            addStatistics(std::make_shared<Statistic>(
                              std::string(kStatGlobalPower) + "_" +
                                  std::to_string(devIndex),
                              std::make_shared<GlobalAccumulator>()),
                          domainInfo->controlledParameter, devIndex);
        }
    }

    void createStatisticsEnergy()
    {
        if (energyReadingType)
        {
            addStatistics(
                std::make_shared<EnergyStatistic>(kStatGlobalEnergyAccum),
                energyReadingType.value());

            // install readings for domain specific statistics
            for (DeviceIndex devIndex = 0;
                 devIndex < domainInfo->maxComponentNumber; ++devIndex)
            {
                addStatistics(std::make_shared<EnergyStatistic>(
                                  std::string(kStatGlobalEnergyAccum) + "_" +
                                  std::to_string(devIndex)),
                              energyReadingType.value(), devIndex);
            }
        }
    }

    void createStatisticsThrottling()
    {
        if (limitReadingType)
        {
            addStatistics(std::make_shared<ThrottlingStatistic>(
                              kStatGlobalThrottling,
                              std::make_shared<GlobalAccumulator>(),
                              domainInfo->capabilities),
                          limitReadingType.value());
        }
    }

    void createStatistics() override final
    {
        createStatisticsPower();
        createStatisticsEnergy();
        createStatisticsThrottling();
    }

    double applyBias(double ptamLimit, DeviceIndex idx)
    {
        double biasedLimit = ptamLimit * limitBiasRelative + limitBiasAbsolute;
        auto componentCapabilities = getComponentCapabilities(idx);
        const auto [min, max] =
            componentCapabilities != nullptr
                ? std::make_pair(componentCapabilities->getMin(),
                                 componentCapabilities->getMax())
                : std::make_pair(domainInfo->capabilities->getMin(),
                                 domainInfo->capabilities->getMax());
        return std::clamp(biasedLimit, min, max);
    }

    /**
     * @brief Finds policy which limit has been used to limit power in
     * Budgeting. It sends onLimitSelection signal for that policy to trigger
     * transition from state Triggered to Selected. For all other policies
     * forces to change policy state from Selected to Triggered, if such
     * transition can be applied.
     */
    void matchPolicyWithSelectedLimit()
    {
        for (const auto& [key, policy] : limitingPolicies)
        {
            if (auto policySp = policy.lock())
            {
                const auto& [componentId, strategy] = key;
                bool strategyLimitInUse = budgeting->isActive(
                    domainInfo->domainId, componentId, strategy);
                policySp->setLimitSelected(strategyLimitInUse);
            }
        }
    }

    /**
     * @brief Creates list with lowest power limits per component id/strategy.
     *
     * @return DomainLimits
     */
    DomainLimits getTriggeredPoliciesLimits()
    {
        DomainLimits lowestLimitPolicies;
        for (auto const& policy : policies)
        {
            auto policyState = policy->getState();
            if (PolicyState::triggered == policyState ||
                PolicyState::selected == policyState)
            {
                // insert limit or return existing one
                const auto& [lowestLimitPolicyIt, islimitInserted] =
                    lowestLimitPolicies.insert(
                        {{policy->getInternalComponentId(),
                          policy->getStrategy()},
                         policy});
                auto& lowestLimitPolicy = lowestLimitPolicyIt->second;

                if (auto policySp = lowestLimitPolicy.lock())
                {
                    if (policy->getLimit() < policySp->getLimit())
                    {
                        lowestLimitPolicy = policy;
                    }
                }
            }
        }
        return lowestLimitPolicies;
    }

    /**
     * @brief Sends power limits to Budgeting: i) when new limit is set, ii)
     * bias settings has been updated.
     *
     * @param limits
     */
    void updateLimits(const DomainLimits& lowestLimitPolicies)
    {
        for (const auto& [key, policy] : lowestLimitPolicies)
        {
            // insert limit or return existing one
            const auto& [limitingPolicyIt, isLimitInserted] =
                limitingPolicies.insert({key, policy});
            auto& limitingPolicy = limitingPolicyIt->second;
            auto limitingPolicySp = limitingPolicy.lock();

            if (auto policySp = policy.lock())
            {
                if (limitingPolicySp &&
                    limitingPolicySp->getId() != policySp->getId())
                {
                    limitingPolicySp->setLimitSelected(false);
                }
                limitingPolicy = policy;
                auto& [componentId, strategy] = key;
                budgeting->setLimit(
                    domainInfo->domainId, componentId,
                    applyBias(policySp->getLimit(), componentId), strategy);
            }
        }
        biasUpdated = false;
    }

    /**
     * @brief Compares list of limits sent to Budgeting with limits sent in the
     * previous cycle. Removes entries which are currently not used for limiting
     * power.
     *
     * @param activeLimits
     */
    void deleteNonActiveLimits(const DomainLimits& activeLimits)
    {
        for (auto it = limitingPolicies.begin(); it != limitingPolicies.end();)
        {
            auto activeLimitsIt = activeLimits.find(it->first);
            if (activeLimitsIt == activeLimits.end() ||
                activeLimitsIt->second.expired())
            {
                const auto& [componentId, strategy] = it->first;
                budgeting->resetLimit(domainInfo->domainId, componentId,
                                      strategy);
                it = limitingPolicies.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void validatePolicies()
    {
        for (std::shared_ptr<nodemanager::PolicyIf> policy : policies)
        {
            policy->validateParameters();
        }
    }

    std::shared_ptr<DomainInfo> createDomainInfo(
        std::string const& objectPath, DomainId id,
        ReadingType controlledParameter,
        std::optional<ReadingType> minCapabilityReading,
        std::optional<ReadingType> maxCapabilityReading,
        const std::shared_ptr<CapabilitiesFactoryIf>& capabilitiesFactory,
        DeviceIndex maxComponentNumber,
        const std::shared_ptr<std::vector<TriggerType>>& triggers,
        std::chrono::milliseconds policyCorrectionTime, DomainId domainId)
    {
        return std::make_shared<DomainInfo>(DomainInfo{
            objectPath + "/Domain/" + enumToStr(kDomainIdNames, id),
            controlledParameter,
            capabilitiesFactory->createDomainCapabilities(
                minCapabilityReading, maxCapabilityReading,
                static_cast<uint32_t>(policyCorrectionTime.count()),
                std::bind(&DomainPower::onCapabilitiesChange, this), domainId),
            id, std::make_shared<std::vector<DeviceIndex>>(), false, triggers,
            maxComponentNumber});
    }

    double getCapabilitiesMax() const override
    {
        return domainInfo->capabilities->getMax();
    }

    double getCapabilitiesMin() const override
    {
        return domainInfo->capabilities->getMin();
    }

    uint32_t getCorrTimeMax() const override
    {
        return domainInfo->capabilities->getMaxCorrectionTimeInMs();
    }

    uint32_t getCorrTimeMin() const override
    {
        return domainInfo->capabilities->getMinCorrectionTimeInMs();
    }

    int setLimitBiasAbsolute(const double& newValue, double& oldValue) override
    {
        verifyRange<errors::OutOfRange>(newValue, kMinBiasValue, kMaxBiasValue);
        limitBiasAbsolute = oldValue = newValue;
        biasUpdated = true;
        return 1;
    }

    double getLimitBiasAbsolute() const override
    {
        return limitBiasAbsolute;
    }

    int setLimitBiasRelative(const double& newValue, double& oldValue) override
    {
        verifyRange<errors::OutOfRange>(newValue, kMinBiasValue, kMaxBiasValue);
        limitBiasRelative = oldValue = newValue;
        biasUpdated = true;
        return 1;
    }

    double getLimitBiasRelative() const override
    {
        return limitBiasRelative;
    }

    std::map<std::string, CapabilitiesValuesMap>
        getAllLimitsCapabilities() const override
    {
        std::map<std::string, CapabilitiesValuesMap> retMap;

        retMap.emplace(domainInfo->capabilities->getName(),
                       domainInfo->capabilities->getValuesMap());
        for (DeviceIndex idx = 0; idx < componentCapabilitiesVector.size();
             ++idx)
        {
            retMap.emplace(componentCapabilitiesVector[idx]->getName(),
                           componentCapabilitiesVector[idx]->getValuesMap());
        }

        return retMap;
    }

    sdbusplus::message::object_path
        createPolicyWithId(PolicyId policyIdArg, PolicyParamsTuple t) override
    {
        try
        {
            PolicyParams params;
            params << t;
            const std::shared_ptr<PolicyIf> policy = createPolicy(
                policyIdArg, PolicyOwner::bmc, params, kNotForcedCreate,
                DbusState::disabled, PolicyEditable::yes, true);
            return sdbusplus::message::object_path{policy->getObjectPath()};
        }
        catch (const std::exception& ex)
        {
            Logger::log<LogLevel::warning>("Cannot create policy, reason: %s",
                                           ex.what());
            throw;
        }
    }

    sdbusplus::message::object_path
        createPolicyForTotalBudget(PolicyId policyIdArg,
                                   PolicyParamsTuple t) override
    {
        try
        {
            PolicyParams params;
            params << t;
            const std::shared_ptr<PolicyIf> policy = createPolicy(
                policyIdArg, PolicyOwner::totalBudget, params, kNotForcedCreate,
                DbusState::enabled, PolicyEditable::yes, true);
            return sdbusplus::message::object_path{policy->getObjectPath()};
        }
        catch (const std::exception& ex)
        {
            Logger::log<LogLevel::warning>(
                "Cannot create total budget policy, reason: %s", ex.what());
            throw;
        }
    }

    /**
     * @brief Returns policy id for the policy which state is set to Selected.
     *
     * @return PolicyId
     */
    sdbusplus::message::object_path getSelectedPolicyId() const override
    {
        std::shared_ptr<PolicyIf> selectedPolicy;
        for (const auto& policy : policies)
        {
            if ((policy->getState() == PolicyState::selected) &&
                ((policy->getComponentId() == kComponentIdAll) ||
                 (!selectedPolicy)))
            {
                selectedPolicy = policy;
            }
        }

        if (selectedPolicy)
        {
            return sdbusplus::message::object_path{
                selectedPolicy->getObjectPath()};
        }

        return sdbusplus::message::object_path{"/"};
    }
};

} // namespace nodemanager
