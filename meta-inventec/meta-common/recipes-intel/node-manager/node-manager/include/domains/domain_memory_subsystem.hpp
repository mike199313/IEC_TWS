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

#include "domain_power.hpp"
#include "policies/policy_factory.hpp"
#include "statistics/global_accumulator.hpp"

namespace nodemanager
{

static const std::shared_ptr<std::vector<TriggerType>>
    kTriggersInDomainMemorySubsystem =
        std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
            TriggerType::always, TriggerType::inletTemperature,
            TriggerType::gpio, TriggerType::hostReset,
            TriggerType::smbalertInterrupt});

class DomainMemorySubsystem : public DomainPower
{
  public:
    DomainMemorySubsystem() = delete;
    DomainMemorySubsystem(const DomainMemorySubsystem&) = delete;
    DomainMemorySubsystem& operator=(const DomainMemorySubsystem&) = delete;
    DomainMemorySubsystem(DomainMemorySubsystem&&) = delete;
    DomainMemorySubsystem& operator=(DomainMemorySubsystem&&) = delete;

    DomainMemorySubsystem(
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
                    ReadingType::dramPower, std::nullopt,
                    ReadingType::dramPackagePowerCapabilitiesMax,
                    ReadingType::dramPowerLimit, ReadingType::dramEnergy,
                    policyFactoryArg, capabilitiesFactory, kMaxCpuNumber,
                    kTriggersInDomainMemorySubsystem,
                    kPolicyMinCorrectionTimeInMs, dbusState)
    {
        registerAvailableComponents<kMaxCpuNumber>(ReadingType::cpuPresence);
    }

    virtual ~DomainMemorySubsystem() = default;

    void createDefaultPolicies() override
    {
        createDmtfPolicies("_Memory", "_Memories");
    }

    static constexpr DomainId ID = DomainId::MemorySubsystem;
};

} // namespace nodemanager
