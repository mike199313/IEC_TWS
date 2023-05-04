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

#include "domain_power.hpp"
#include "statistics/global_accumulator.hpp"

namespace nodemanager
{

static const std::shared_ptr<std::vector<TriggerType>> kTriggersInDomainPcie =
    std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
        TriggerType::always, TriggerType::inletTemperature, TriggerType::gpio,
        TriggerType::hostReset, TriggerType::smbalertInterrupt});

class DomainPcie : public DomainPower
{
  public:
    DomainPcie() = delete;
    DomainPcie(const DomainPcie&) = delete;
    DomainPcie& operator=(const DomainPcie&) = delete;
    DomainPcie(DomainPcie&&) = delete;
    DomainPcie& operator=(DomainPcie&&) = delete;

    DomainPcie(
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
        DomainPower(
            busArg, objServerArg, objectPathArg, devicesManagerArg,
            gpioProviderArg, triggersManagerArg, budgetingArg, ID,
            ReadingType::pciePower,
            (Config::getInstance().getGeneralPresets().acceleratorsInterface ==
             kPldmInterfaceName)
                ? std::make_optional<ReadingType>(
                      ReadingType::pciePowerCapabilitiesMin)
                : std::nullopt,
            ReadingType::pciePowerCapabilitiesMax, std::nullopt, std::nullopt,
            policyFactoryArg, capabilitiesFactory, kMaxPcieNumber,
            kTriggersInDomainPcie, kPolicyMinCorrectionTimeInMs, dbusState)
    {
        registerAvailableComponents<kMaxPcieNumber>(ReadingType::pciePresence);
    }

    virtual ~DomainPcie() = default;

    void createDefaultPolicies() override
    {
        createDmtfPolicies("_Accelerator", "_Accelerators");
        createSmbalertPolicy();
    }

    static constexpr DomainId ID = DomainId::Pcie;

  private:
    void createSmbalertPolicy()
    {
        auto policy = createPolicy(
            kSmbalertPowerPolicyId, PolicyOwner::internal,
            PolicyParams{
                kInternalSmartPolicyCorrectionTime, kZeroWattLimit,
                kMinimumStatReportingPeriod, PolicyStorage::volatileStorage,
                PowerCorrectionType::automatic, LimitException::noAction,
                PolicySuspendPeriods{}, PolicyThresholds{},
                uint8_t{kAllDevices}, kZeroTriggerLimit, "SMBAlertInterrupt"},
            kForcedCreate, DbusState::enabled, PolicyEditable::yes, false);
    }
};

} // namespace nodemanager
