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
#include "devices_manager/devices_manager.hpp"
#include "domains/capabilities/domain_capabilities.hpp"
#include "domains/domain_types.hpp"
#include "loggers/log.hpp"
#include "policy_dbus_properties.hpp"
#include "policy_enums.hpp"
#include "policy_if.hpp"
#include "policy_state.hpp"
#include "policy_storage_management.hpp"
#include "policy_types.hpp"
#include "readings/reading_event.hpp"
#include "statistics/statistic.hpp"
#include "statistics/statistics_provider.hpp"
#include "triggers/triggers_manager.hpp"
#include "utility/dbus_enable_if.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/ranges.hpp"
#include "utility/types.hpp"

#include <memory>

namespace nodemanager
{

using DeleteCallback = std::function<void(const PolicyId policyId)>;

static constexpr const auto kStatPolicyTrigger = "Trigger";       //[Celsius]
static constexpr const auto kStatPolicyThrottling = "Throttling"; //[%]
static constexpr const auto kForceHighestThrottlingLevel = 0;

class Policy : public PolicyIf,
               public StatisticsProvider,
               protected PolicyDbusProperties,
               public std::enable_shared_from_this<Policy>
{
  public:
    Policy() = delete;
    Policy(const Policy&) = delete;
    Policy& operator=(const Policy&) = delete;
    Policy(Policy&&) = delete;
    Policy& operator=(Policy&&) = delete;

    Policy(PolicyId id, PolicyOwner ownerArg,
           std::shared_ptr<DevicesManagerIf> devicesManagerArg,
           std::shared_ptr<GpioProviderIf> gpioProviderArg,
           std::shared_ptr<TriggersManagerIf> triggersManagerArg,
           std::shared_ptr<PolicyStorageManagementIf> storageManagementArg,
           uint16_t statReportingPeriodArg,
           std::shared_ptr<sdbusplus::asio::connection> busArg,
           std::shared_ptr<sdbusplus::asio::object_server> objectServerArg,
           std::shared_ptr<DomainInfo>& domainInfoArg,
           const DeleteCallback deleteCallbackArg, DbusState dbusState,
           PolicyEditable editableArg, bool allowDeleteArg) :
        PolicyIf(dbusState),
        StatisticsProvider(devicesManagerArg), PolicyDbusProperties(id),
        domainInfo(domainInfoArg), bus(busArg), objectServer(objectServerArg),
        devicesManager(devicesManagerArg), gpioProvider(gpioProviderArg),
        editable(editableArg), allowDelete(allowDeleteArg),
        triggersManager(triggersManagerArg),
        storageManagement(storageManagementArg),
        deleteCallback(deleteCallbackArg)
    {
        try
        {
            std::regex validPolicyId("^[A-Za-z0-9_]{1,255}$");
            if (!std::regex_match(id, validPolicyId))
            {
                Logger::log<LogLevel::warning>(
                    "Policy id is not valid dbus path or is longer than 255");
                throw errors::InvalidPolicyId();
            }
            objectPath = domainInfo->objectPath + std::string{"/Policy/"} + id;

            setVerifyDbusSetFunctions();
            setPostDbusSetFunctions();
            owner.set(toEnum<errors::PoliciesCannotBeCreated>(kPolicyOwner,
                                                              ownerArg));

            domainId.set(enumToStr(kDomainIdNames, domainInfo->domainId));

            policyState.setCustomGetter(
                [this]() { return policyStateIf->getState(); });

            shortObjectPath = objectPath.substr(strlen(kRootObjectPath));
            readingEvent =
                std::make_shared<ReadingEvent>([this](double incomingValue) {});
            initializeDbusInterfaces();
            DbusEnableIf::initializeDbusInterfaces(
                dbusInterfaces,
                std::bind(&Policy::saveOrDeletePolicyFile, this, false));
            StatisticsProvider::initializeDbusInterfaces(dbusInterfaces);
            if (owner.get() != PolicyOwner::internal)
            {
                Logger::log<LogLevel::info>("Policy %s created",
                                            getShortObjectPath());
            }
        }
        catch (const errors::InvalidPolicyId& e)
        {
            throw e;
        }
        catch (...)
        {
            throw errors::PoliciesCannotBeCreated();
        }
    }

    virtual ~Policy()
    {
        uninstallTrigger();
        freeGpio();
        Logger::log<LogLevel::info>("Policy %s removed", getShortObjectPath());
    }

    virtual void initialize()
    {
        policyStateIf->initialize(shared_from_this(), bus);
    }

    virtual void onStateChanged()
    {
        setState(policyStateIf->onEnabled(isEnabledOnDbus()));
        setState(policyStateIf->onParentEnabled(isParentEnabled()));
    }

    virtual void run() override
    {
    }

    PolicyId getId() const override
    {
        return policyId.get();
    }

    virtual void verifyPolicy(const PolicyParams& params) const override
    {
        verifyReadingSource();
        verifyParameters(params);
    }

    virtual void verifyParameters(const PolicyParams& params) const override
    {
        verifyPolicyStorage(params.policyStorage);
        TriggerType triggerTypeEnum =
            triggerType.verifyFromDbusType(params.triggerType);
        verifyTriggerType(triggerTypeEnum);
        verifyComponentId(params.componentId);
        verifyLimit(params.limit, params.componentId, triggerTypeEnum);
        verifyTriggerLimit(params.triggerLimit, triggerTypeEnum);
        verifyStatReportingPeriod(params.statReportingPeriod);

        // TODO verify if we are able to modify parameters when policy is
        // enabled else throw errors::PoliciesCannotBeCreated
    }

    virtual void updateParams(const PolicyParams& params)
    {
        // update all parameters exposed by this class
        policyStorage.set(params.policyStorage);
        componentId.set(params.componentId);
        statReportingPeriod.set(params.statReportingPeriod);
        triggerType.set(params.triggerType);
        triggerLimit.set(params.triggerLimit);
        limit.set(params.limit);

        if (isRunning())
        {
            updateTrigger();
        }

        setState(policyStateIf->onEnabled(isEnabledOnDbus()));
        setState(policyStateIf->onParentEnabled(isParentEnabled()));

        Logger::log<LogLevel::info>("Policy %s updated", getShortObjectPath());
    }

    virtual void adjustCorrectableParameters()
    {
    }

    void validateParameters() final override
    {
        PolicyParams params;
        getPolicyParams(params);
        try
        {
            verifyPolicy(params);
        }
        catch (const sdbusplus::exception_t& e)
        {
            if (owner.get() != PolicyOwner::internal)
            {
                RedfishLogger::logPolicyAttributeIncorrect(getShortObjectPath(),
                                                           e.description());
                Logger::log<LogLevel::info>("%s - Policy %s validation failed",
                                            e.description(),
                                            getShortObjectPath());
            }
            setState(policyStateIf->onParametersValidation(false));
            return;
        }
        setState(policyStateIf->onParametersValidation(true));
    }

    uint16_t getLimit() const
    {
        return limit.get();
    }

    void setLimit(uint16_t value)
    {
        limit.set(value);
    }

    void setLimitSelected(bool isLimitSelected)
    {
        setState(policyStateIf->onLimitSelection(isLimitSelected));
    }

    PolicyState getState() const override
    {
        return policyStateIf->getState();
    }

    DeviceIndex getComponentId() const
    {
        return componentId.get();
    }

    DeviceIndex getInternalComponentId() const
    {
        switch (domainInfo->domainId)
        {
            case DomainId::CpuSubsystem:
            case DomainId::MemorySubsystem:
            case DomainId::Pcie:
                return getComponentId();
            default:
                // For compound domains componentId is ignored
                return kComponentIdAll;
        }
    }

    void installTrigger() override
    {
        if (!trigger)
        {
            updateTrigger();
        }
    }

    void uninstallTrigger() override
    {
        if (trigger)
        {
            devicesManager->unregisterReadingConsumer(trigger);
            trigger = nullptr;
        }
    }

    const std::string& getShortObjectPath() const
    {
        return shortObjectPath;
    }

    const std::string& getObjectPath() const
    {
        return objectPath;
    }

    PolicyOwner getOwner() const
    {
        return owner.get();
    }

    virtual nlohmann::json toJson() const override
    {
        PolicyParams params;
        getPolicyParams(params);
        return {{"domainId", domainInfo->domainId},
                {"owner", getOwner()},
                {"isEnabled", isEnabledOnDbus()},
                {"policyParams", params}};
    }

    void postCreate() override
    {
        saveOrDeletePolicyFile(true);
        reserveGpio();
    }

  protected:
    std::shared_ptr<DomainInfo> domainInfo;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string objectPath;
    DbusInterfaces dbusInterfaces{objectPath, objectServer};
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::string shortObjectPath;
    std::function<void(bool)> postParameterUpdate =
        [this](bool storageChanged) {
            validateParameters();
            saveOrDeletePolicyFile(storageChanged);
            reserveGpio();
        };
    const PolicyEditable editable;
    bool allowDelete;

    virtual bool isEditable() const override
    {
        return editable == PolicyEditable::yes;
    }

    void setState(std::unique_ptr<PolicyStateIf> state)
    {
        if (state)
        {
            policyStateIf = std::move(state);
            policyStateIf->initialize(shared_from_this(), bus);
        }
    }

    virtual void getPolicyParams(PolicyParams& params) const
    {
        params.limit = limit.get();
        params.statReportingPeriod = statReportingPeriod.get();
        params.policyStorage = policyStorage.get();
        params.componentId = componentId.get();
        params.triggerLimit = triggerLimit.get();
        params.triggerType = enumToStr(kTriggerTypeNames, triggerType.get());
    }

  private:
    std::shared_ptr<TriggersManagerIf> triggersManager;
    std::shared_ptr<PolicyStorageManagementIf> storageManagement;
    bool isStored;
    bool isVisible;
    bool isValid;
    bool isReadingAvailable;
    std::shared_ptr<Trigger> trigger;
    std::shared_ptr<ReadingEvent> readingEvent;
    static std::set<PolicyId> uniqueIds;
    const DeleteCallback deleteCallback;
    std::unique_ptr<PolicyStateIf> policyStateIf =
        std::make_unique<PolicyStateDisabled>();
    std::optional<DeviceIndex> gpioReserved = std::nullopt;

    virtual void verifyLimit(uint16_t limitArg, DeviceIndex componentIdArg,
                             TriggerType triggerTypeArg) const = 0;

    void setVerifyDbusSetFunctions()
    {
        limit.setVerifyDbusSetFunction([this](uint16_t limitArg) {
            this->verifyLimit(limitArg, getComponentId(), triggerType.get());
        });

        policyStorage.setVerifyDbusSetFunction(
            [this](PolicyStorage policyStorageArg) {
                this->verifyPolicyStorage(policyStorageArg);
            });

        componentId.setVerifyDbusSetFunction(
            [this](DeviceIndex componentIdArg) {
                this->verifyComponentId(componentIdArg);
            });

        statReportingPeriod.setVerifyDbusSetFunction(
            [this](uint16_t statReportingPeriodArg) {
                this->verifyStatReportingPeriod(statReportingPeriodArg);
            });

        triggerType.setVerifyDbusSetFunction(
            [this](TriggerType triggerTypeArg) {
                this->verifyTriggerType(triggerTypeArg);
                this->verifyTriggerLimit(triggerLimit.get(), triggerTypeArg);
            });

        triggerLimit.setVerifyDbusSetFunction([this](uint16_t triggerLimitArg) {
            this->verifyTriggerLimit(triggerLimitArg, triggerType.get());
        });
    }

    void verifyReadingSource() const
    {
        if (domainInfo->requiredReadingUnavailable)
        {
            throw errors::ReadingSourceUnavailable();
        }
    }

    DeviceIndex triggerLimitToGpioIndex(const uint16_t triggerLimitArg) const
    {
        return safeCast<DeviceIndex>(triggerLimitArg & 0x7FFF, kAllDevices);
    }

    /**
     * @brief Verifies if provided component id is with range of max
     * supported componenets per domain and if is within currently
     * available components in the domain. Throws errors::InvalidComponentId
     * if any of criteria are not met.
     *
     * @param componentIdArg
     * @return void
     */
    void verifyComponentId(DeviceIndex componentIdArg) const
    {
        if (kComponentIdAll == componentIdArg ||
            domainInfo->maxComponentNumber == kComponentIdIgnored)
        {
            return;
        }

        if (!(componentIdArg < domainInfo->maxComponentNumber &&
              std::find(domainInfo->availableComponents->begin(),
                        domainInfo->availableComponents->end(),
                        componentIdArg) !=
                  domainInfo->availableComponents->end()))
        {
            throw errors::InvalidComponentId();
        }
    }

    void verifyPolicyStorage(const PolicyStorage& policyStorageArg) const
    {
        toEnum<errors::InvalidPolicyStorage>(kPolicyStorage, policyStorageArg);
    }

    void verifyTriggerLimit(uint16_t triggerLimitArg,
                            TriggerType triggerTypeArg) const
    {
        if (gpioReserved && (triggerTypeArg == TriggerType::gpio) &&
            (triggerLimitToGpioIndex(triggerLimitArg) == *gpioReserved))
        {
            return;
        }
        if (triggerTypeArg != TriggerType::always)
        {
            std::shared_ptr<TriggerCapabilities> triggerCap =
                triggersManager->getTriggerCapabilities(triggerTypeArg);
            if (!triggerCap->isTriggerLevelValid(triggerLimitArg))
            {
                throw errors::TriggerValueOutOfRange();
            }
        }
    }

    void verifyStatReportingPeriod(uint16_t statReportingPeriodArg) const
    {
        verifyRange<errors::StatRepPeriodOutOfRange>(
            statReportingPeriodArg,
            domainInfo->capabilities->getMinStatReportingPeriod(),
            domainInfo->capabilities->getMaxStatReportingPeriod());
    }

    void verifyTriggerType(const TriggerType& triggerTypeArg) const
    {
        if (!triggersManager->isTriggerAvailable(triggerTypeArg))
        {
            throw errors::UnsupportedPolicyTriggerType();
        }

        for (auto triggerSupported : *(domainInfo->triggers))
        {
            if (triggerTypeArg == triggerSupported)
            {
                return;
            }
        }

        throw errors::UnsupportedPolicyTriggerType();
    }

    void setPostDbusSetFunctions()
    {
        limit.setPostDbusSetFunction(std::bind(postParameterUpdate, false));

        policyStorage.setPostDbusSetFunction(
            std::bind(postParameterUpdate, true));

        componentId.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));

        statReportingPeriod.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));

        triggerLimit.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));

        triggerType.setPostDbusSetFunction(
            std::bind(postParameterUpdate, false));
    }

    void updateTrigger()
    {
        auto triggerTypeTmp = triggerType.get();
        if (triggerTypeTmp == TriggerType::always)
        {
            setState(
                policyStateIf->onTriggerAction(TriggerActionType::trigger));
        }
        else
        {
            // Unregister reading in DeviceManager in case trigger exists
            if (trigger)
            {
                devicesManager->unregisterReadingConsumer(trigger);
            }

            trigger = triggersManager->createTrigger(
                triggerTypeTmp, triggerLimit.get(),
                [this, triggerTypeTmp](TriggerActionType at) {
                    setState(policyStateIf->onTriggerAction(at));
                    if (triggerTypeTmp == TriggerType::hostReset ||
                        triggerTypeTmp == TriggerType::cpuUtilization)
                    {
                        if (at == TriggerActionType::deactivate)
                        {
                            disableStatisticsCalculation();
                        }
                        else if (at == TriggerActionType::trigger)
                        {
                            enableStatisticsCalculation();
                        }
                    }
                });

            if (triggerTypeTmp == TriggerType::gpio)
            {
                devicesManager->registerReadingConsumer(
                    trigger, Trigger::toReadingType(triggerTypeTmp),
                    triggerLimitToGpioIndex(triggerLimit.get()));
            }
            else if (triggerTypeTmp == TriggerType::cpuUtilization &&
                     domainInfo->domainId == DomainId::CpuSubsystem)
            {
                devicesManager->registerReadingConsumer(
                    trigger, Trigger::toReadingType(triggerTypeTmp),
                    getComponentId());
            }
            else
            {
                devicesManager->registerReadingConsumer(
                    trigger, Trigger::toReadingType(triggerTypeTmp));
            }
        }
    }

    void saveOrDeletePolicyFile(bool policyStorageChanged)
    {
        if (policyStorage.get() == PolicyStorage::persistentStorage)
        {
            storageManagement->policyWrite(getId(), toJson());
        }
        else if (policyStorageChanged)
        {
            storageManagement->policyDelete(getId());
        }
    }

    void freeGpio()
    {
        if (gpioReserved)
        {
            gpioProvider->freeGpio(*gpioReserved);
            gpioReserved = std::nullopt;
        }
    }

    void reserveGpio()
    {
        freeGpio();

        if (triggerType.get() == TriggerType::gpio)
        {
            auto gpioToReserve = triggerLimitToGpioIndex(triggerLimit.get());
            if (gpioProvider->reserveGpio(gpioToReserve))
            {
                gpioReserved = gpioToReserve;
            }
        }
    }

    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.PolicyAttributes",
            [this](auto& policyAttributesInterface) {
                registerProperties(policyAttributesInterface, isEditable());

                policyAttributesInterface.register_method(
                    "Update", [this](PolicyParamsTuple t) {
                        try
                        {
                            if (!isEditable())
                            {
                                throw errors::OperationNotPermitted();
                            }
                            PolicyParams params;
                            params << t;
                            bool policyStorageChanged =
                                (policyStorage.get() != params.policyStorage);
                            verifyParameters(params);
                            updateParams(params);
                            postParameterUpdate(policyStorageChanged);
                        }
                        catch (const std::exception& ex)
                        {
                            Logger::log<LogLevel::warning>(
                                "Cannot update policy, reason: %s", ex.what());
                            throw;
                        }
                    });
            });

        dbusInterfaces.addInterface(
            "xyz.openbmc_project.Object.Delete", [this](auto& iface) {
                iface.register_method("Delete", [this]() {
                    Logger::log<LogLevel::info>(
                        "Triggering delete of policy %s", getShortObjectPath());
                    if (!allowDelete)
                    {
                        throw errors::OperationNotPermitted();
                    }
                    bus->get_io_context().post(
                        [id = getId(), shortObjectPath = getShortObjectPath(),
                         deleteFun = deleteCallback,
                         storage = storageManagement]() {
                            Logger::log<LogLevel::info>("Deleting policy, %s",
                                                        shortObjectPath);
                            storage->policyDelete(id);
                            if (deleteFun)
                            {
                                deleteFun(id);
                            }
                        });
                });
            });
    }
};
std::set<PolicyId> Policy::uniqueIds = {};
} // namespace nodemanager
