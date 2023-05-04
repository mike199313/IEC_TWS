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

#include "budgeting/budgeting.hpp"
#include "budgeting/efficiency_helper.hpp"
#include "budgeting/simple_domain_budgeting.hpp"
#include "common_types.hpp"
#include "control/control.hpp"
#include "devices_manager/devices_manager.hpp"
#include "efficiency_control.hpp"
#include "flow_control.hpp"
#include "policies/policy.hpp"
#include "ptam.hpp"
#include "readings/reading_type.hpp"
#include "regulator_p.hpp"
#include "sensors/sensor_readings_manager.hpp"
#include "smart_supervisor.hpp"
#include "statistics/global_accumulator.hpp"
#include "statistics/statistics_provider.hpp"
#include "status_monitor.hpp"
#include "throttling_events/throttling_log_collector.hpp"
#include "triggers/triggers_manager.hpp"
#include "utility/dbus_enable_if.hpp"
#include "utility/dbus_interfaces.hpp"
#include "utility/diagnostics.hpp"
#include "utility/performance_monitor.hpp"

#include <systemd/sd-daemon.h>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

namespace nodemanager
{

struct SimpleDomainBudgetingConfig
{
    RaplDomainId raplDomainId;
    double regulatorPCoeff;
    ReadingType regulatorFeedbackReading;
    ReadingType efficiencyReading;
    std::chrono::milliseconds efficiencyAveragingPeriod;
    double budgetCorrection;
    DomainId capabilitiesDomainId;
};

static const std::vector<SimpleDomainBudgetingConfig>
    kSimpleDomainDistributorsConfig = {
        {RaplDomainId::memorySubsystem, 0.4, ReadingType::dcPlatformPower,
         ReadingType::dramPower, std::chrono::milliseconds{500}, 0.2,
         DomainId::MemorySubsystem},

        {RaplDomainId::pcie, 0.4, ReadingType::dcPlatformPower,
         ReadingType::pciePower, std::chrono::milliseconds{500}, 0.2,
         DomainId::Pcie}};

static constexpr const auto kStatGlobalInletTemp =
    "Inlet temperature"; //[Celsius]
static constexpr const auto kStatGlobalOutletTemp =
    "Outlet temperature"; //[Celsius]
static constexpr const auto kStatGlobalVolumetricAirflow =
    "Volumetric airflow"; //[1/10th of CFM]
static constexpr const auto kStatGlobalChassisPower = "Chassis power"; //[Watts]
static constexpr const auto kSmartParametersDirectory =
    "/sys/devices/platform/smart";

static const std::chrono::milliseconds kLoopPeriod{100};

class NodeManager : public RunnerIf, DbusEnableIf
{
  public:
    NodeManager() = delete;
    NodeManager(const NodeManager&) = delete;
    NodeManager& operator=(const NodeManager&) = delete;
    NodeManager(NodeManager&&) = delete;
    NodeManager& operator=(NodeManager&&) = delete;

    NodeManager(boost::asio::io_context& iocArg,
                std::shared_ptr<sdbusplus::asio::connection> busArg,
                std::string const& objectPathArg) :
        DbusEnableIf(
            (Config::getInstance().getGeneralPresets().policyControlEnabled)
                ? DbusState::enabled
                : DbusState::disabled),
        ioc(iocArg), bus(busArg),
        objectServer(std::make_shared<sdbusplus::asio::object_server>(bus)),
        objectPath(objectPathArg), tickTimer(ioc),
        loopTimer(bus->get_io_context()), loopTimeout(kLoopPeriod)
    {
        throttlingLogCollector = std::make_shared<ThrottlingLogCollector>(ioc);
        gpioProvider = std::make_shared<GpioProvider>();
        sensorReadingsManager = std::make_shared<SensorReadingsManager>();
        smartSupervisor = std::make_unique<SmartSupervisor>(
            bus, objectServer, objectPath, sensorReadingsManager,
            kSmartParametersDirectory);
        devicesManager = std::make_shared<DevicesManager>(
            bus, sensorReadingsManager,
            std::make_shared<HwmonFileProvider>(bus), gpioProvider,
            std::make_shared<PldmEntityProvider>(bus));
        statusMonitor = std::make_shared<StatusMonitor>(
            devicesManager, std::make_shared<StatusMonitorActions>());
        efficiencyControl = std::make_shared<EfficiencyControl>(devicesManager);
        control = std::make_shared<Control>(devicesManager);
        budgeting = makeBudgeting();
        triggersManager = std::make_shared<TriggersManager>(
            objectServer, objectPath, gpioProvider);
        policyStorageManagement = std::make_shared<PolicyStorageManagement>();
        ptam = std::make_unique<Ptam>(bus, objectServer, objectPath,
                                      devicesManager, gpioProvider, budgeting,
                                      efficiencyControl, triggersManager,
                                      policyStorageManagement);
        diagnostics = std::make_shared<Diagnostics>(
            bus, objectPath, devicesManager, throttlingLogCollector);
        statisticsProvider =
            std::make_unique<StatisticsProvider>(devicesManager);

        statisticsProvider->addStatistics(
            std::make_shared<Statistic>(kStatGlobalInletTemp,
                                        std::make_shared<GlobalAccumulator>()),
            ReadingType::inletTemperature);
        statisticsProvider->addStatistics(
            std::make_shared<Statistic>(kStatGlobalOutletTemp,
                                        std::make_shared<GlobalAccumulator>()),
            ReadingType::outletTemperature);
        statisticsProvider->addStatistics(
            std::make_shared<Statistic>(kStatGlobalVolumetricAirflow,
                                        std::make_shared<GlobalAccumulator>()),
            ReadingType::volumetricAirflow);
        statisticsProvider->addStatistics(
            std::make_shared<Statistic>(kStatGlobalChassisPower,
                                        std::make_shared<GlobalAccumulator>()),
            ReadingType::totalChassisPower);
        statisticsProvider->initializeDbusInterfaces(dbusInterfaces);

        syncSimpleDomainBudgetingCapabilities();
        initializeDbusInterfaces();
        DbusEnableIf::initializeDbusInterfaces(dbusInterfaces);
        DbusEnableIf::setParentRunning(true);
        onStateChanged();
    }

    ~NodeManager() = default;

    virtual void onStateChanged() override
    {
        Logger::log<LogLevel::info>("NM is running: %b", isRunning());
        ptam->setParentRunning(isRunning());
    }

    void run() override final
    {
        auto perf1 = std::make_shared<Perf>("NodeManager-run-between",
                                            std::chrono::milliseconds{150});

        loopTimer.expires_after(loopTimeout);
        loopTimer.async_wait([this,
                              perf1](const boost::system::error_code& ec) {
            perf1->stopMeasure();

            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }

            // TODO: consider running DeviceManager with seperate timer to
            // decrease DBus performance impact on Node Manager logic
            // mechanism

            auto perf2 =
                Perf("NodeManager-run-duration", std::chrono::milliseconds{50});

            devicesManager->run();
            ptam->run();
            budgeting->run();
            control->run();
            statusMonitor->run();
            smartSupervisor->run();

            ptam->postRun();

            perf2.stopMeasure();
            sd_notify(0, "WATCHDOG=1");

            run();
        });
    }

    std::shared_ptr<Diagnostics> getDiagnostics()
    {
        return diagnostics;
    }

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string const& objectPath;
    boost::asio::deadline_timer tickTimer;
    std::shared_ptr<StatusMonitor> statusMonitor;
    std::shared_ptr<GpioProvider> gpioProvider;
    std::shared_ptr<DevicesManager> devicesManager;
    std::shared_ptr<Control> control;
    std::shared_ptr<EfficiencyControl> efficiencyControl;
    std::shared_ptr<Budgeting> budgeting;
    std::shared_ptr<TriggersManager> triggersManager;
    std::shared_ptr<PolicyStorageManagement> policyStorageManagement;
    std::unique_ptr<Ptam> ptam;
    boost::asio::steady_timer loopTimer;
    std::chrono::milliseconds loopTimeout;
    PropertyPtr<std::underlying_type_t<NmHealth>> health;
    DbusInterfaces dbusInterfaces{objectPath, objectServer};
    std::unique_ptr<StatisticsProvider> statisticsProvider;
    std::shared_ptr<Diagnostics> diagnostics;
    std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager;
    std::unique_ptr<SmartSupervisor> smartSupervisor;
    std::shared_ptr<ThrottlingLogCollector> throttlingLogCollector;

    std::shared_ptr<Budgeting> makeBudgeting()
    {
        SimpleDomainDistributors simpleDomainDistributors;

        for (const auto& config : kSimpleDomainDistributorsConfig)
        {
            simpleDomainDistributors.emplace_back(
                config.raplDomainId,
                std::make_unique<SimpleDomainBudgeting>(
                    std::move(std::make_unique<RegulatorP>(
                        devicesManager, config.regulatorPCoeff,
                        config.regulatorFeedbackReading)),
                    std::move(std::make_unique<EfficiencyHelper>(
                        devicesManager, config.efficiencyReading,
                        config.efficiencyAveragingPeriod)),
                    config.budgetCorrection));
        }

        auto compoundBudgeting = std::make_unique<CompoundDomainBudgeting>(
            std::move(simpleDomainDistributors));

        // TODO: consider removing control as node_manager class member as it
        // could only be used by budgeting, could make local unique_ptr here
        // and move to budgeting

        return std::make_shared<Budgeting>(
            devicesManager, std::move(compoundBudgeting), control);
    }

    void syncSimpleDomainBudgetingCapabilities()
    {
        SimpleDomainCapabilities simpleDomainCapabilities;

        for (const auto& config : kSimpleDomainDistributorsConfig)
        {
            simpleDomainCapabilities.emplace(
                config.raplDomainId,
                ptam->getDomainCapabilities(config.capabilitiesDomainId));
        }

        budgeting->updateSimpleDomainBudgeting(simpleDomainCapabilities);
    }

    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.NodeManager", [this](auto& iface) {
                iface.register_property_r(
                    "Version", std::string(),
                    sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return std::string("1.0"); });
                iface.register_property_r(
                    "Health", std::underlying_type_t<NmHealth>(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) {
                        if (diagnostics)
                        {
                            return std::underlying_type_t<NmHealth>(
                                diagnostics->getHealth());
                        }
                        return std::underlying_type_t<NmHealth>(
                            NmHealth::warning);
                    });
                iface.register_property_r(
                    "MaxNumberOfPolicies", uint8_t(),
                    sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return kNodeManagerMaxPolicies; });
            });
    }
};

} // namespace nodemanager
