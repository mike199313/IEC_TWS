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

#include "balancer.hpp"
#include "common_types.hpp"
#include "config/config.hpp"
#include "devices_manager/devices_manager.hpp"
#include "flow_control.hpp"
#include "loggers/log.hpp"
#include "scalability/cpu_scalability.hpp"
#include "scalability/proportional_capabilites_scalability.hpp"
#include "scalability/proportional_pcie_scalability.hpp"
#include "scalability/total_power_scalability.hpp"
#include "utility/performance_monitor.hpp"

#include <iostream>
#include <map>

namespace nodemanager
{

class ControlIf
{
  public:
    virtual ~ControlIf() = default;
    virtual void setBudget(RaplDomainId domainId,
                           const std::optional<Limit>& limit) = 0;
    virtual void setComponentBudget(RaplDomainId domainId,
                                    DeviceIndex componentId,
                                    const std::optional<Limit>& limit) = 0;
    virtual bool isDomainLimitActive(RaplDomainId domainId) const = 0;
    virtual bool isComponentLimitActive(RaplDomainId domainId,
                                        DeviceIndex componentId) const = 0;
};

class Control : public ControlIf, public RunnerIf
{
  public:
    Control() = delete;
    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;
    Control(Control&&) = delete;
    Control& operator=(Control&&) = delete;

    Control(std::shared_ptr<DevicesManagerIf> devicesManagerArg) :
        devicesManager(devicesManagerArg)
    {
        installBalancers();
    }

    ~Control() = default;

    void run() override final
    {
        auto perf = Perf("Control-run-duration", std::chrono::milliseconds{20});
        for (auto const& [domain, balancer] : domainBalancers)
        {
            balancer->run();
        }
    }

    void setBudget(RaplDomainId domainId,
                   const std::optional<Limit>& limit) override
    {
        try
        {
            domainBalancers.at(domainId)->setDomainPowerBudget(limit);
        }
        catch (const std::out_of_range& e)
        {
            Logger::log<LogLevel::error>(
                "[Control]: Controllable domain [%u] is not supported",
                static_cast<unsigned>(domainId));
        }
    }

    void setComponentBudget(RaplDomainId domainId, DeviceIndex componentId,
                            const std::optional<Limit>& limit) override
    {
        try
        {
            domainBalancers.at(domainId)->setComponentLimit(componentId, limit);
        }
        catch (const std::out_of_range& e)
        {
            Logger::log<LogLevel::error>(
                "[Control]: Controllable domain [%u] is not supported",
                static_cast<unsigned>(domainId));
        }
    }

    bool isDomainLimitActive(RaplDomainId domainId) const override
    {
        try
        {
            return domainBalancers.at(domainId)->isDomainLimitActive();
        }
        catch (const std::out_of_range& e)
        {
            Logger::log<LogLevel::error>(
                "[Control]: Controllable domain [%u] is not supported",
                static_cast<unsigned>(domainId));
        }

        return false;
    }

    bool isComponentLimitActive(RaplDomainId domainId,
                                DeviceIndex componentId) const override
    {
        try
        {
            return domainBalancers.at(domainId)->isComponentLimitActive(
                componentId);
        }
        catch (const std::out_of_range& e)
        {
            Logger::log<LogLevel::error>(
                "[Control]: Controllable domain [%u] is not supported",
                static_cast<unsigned>(domainId));
        }

        return false;
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::unordered_map<RaplDomainId, std::unique_ptr<Balancer>> domainBalancers;

    void installBalancers()
    {
        // DcPlatform balancer
        domainBalancers.emplace(RaplDomainId::dcTotalPower,
                                std::make_unique<Balancer>(
                                    devicesManager, KnobType::DcPlatformPower,
                                    std::make_shared<TotalPowerScalability>()));

        // Cpu balancer
        domainBalancers.emplace(
            RaplDomainId::cpuSubsystem,
            std::make_unique<Balancer>(
                devicesManager, KnobType::CpuPackagePower,
                std::make_shared<CpuScalability>(
                    devicesManager, Config::getInstance()
                                        .getGeneralPresets()
                                        .cpuPerformanceOptimization)));

        // Dram balancer
        domainBalancers.emplace(
            RaplDomainId::memorySubsystem,
            std::make_unique<Balancer>(
                devicesManager, KnobType::DramPower,
                std::make_shared<ProportionalDramScalability>(devicesManager)));

        // Pci balancer
        domainBalancers.emplace(
            RaplDomainId::pcie,
            std::make_unique<Balancer>(
                devicesManager, KnobType::PciePower,
                std::make_shared<ProportionalPcieScalability>(devicesManager)));
    }
}; // namespace nodemanager

} // namespace nodemanager
