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
#include "flow_control.hpp"
#include "sensors/sensor_readings_manager.hpp"
#include "utility/dbus_enable_if.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/dbus_interfaces.hpp"
#include "utility/property_wrapper.hpp"
#include "utility/ranges.hpp"

#include <fstream>
#include <future>
#include <memory>
#include <thread>
#include <unordered_map>
#include <variant>

namespace nodemanager
{

static constexpr const auto kSmartDbusInterfaceName =
    "xyz.openbmc_project.NodeManager.Smart";

class SmartSupervisor : public RunnerIf
{
  public:
    SmartSupervisor() = delete;
    SmartSupervisor(const SmartSupervisor&) = delete;
    SmartSupervisor& operator=(const SmartSupervisor&) = delete;
    SmartSupervisor(SmartSupervisor&&) = delete;
    SmartSupervisor& operator=(SmartSupervisor&&) = delete;
    virtual ~SmartSupervisor() = default;
    SmartSupervisor(
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<sdbusplus::asio::object_server>& objectServerArg,
        std::string const& objectPathArg,
        const std::shared_ptr<SensorReadingsManagerIf>& sensorsManagerArg,
        const std::string& parametersDirArg) :
        objectPath(objectPathArg + "/SmaRTSupervisor"),
        dbusInterfaces(objectPath, objectServerArg),
        sensorsManager(sensorsManagerArg), parametersDir(parametersDirArg)
    {
        setVerifyDbusSetFunctions();
        setDefaultPropertiesValues();
        setPostDbusSetFunctions();
        initializeDbusInterfaces();
    }

    void run() override final
    {
        auto perf1 = std::make_shared<Perf>("SmartSupervisor-run-duration",
                                            std::chrono::milliseconds{10});

        kernelModulePresent.set(isKernelModule());

        if (const auto newPowerState = sensorsManager->isPowerStateOn();
            newPowerState != prevPowerStateOn)
        {
            prevPowerStateOn = newPowerState;
            bool newSmartEnabled =
                Config::getInstance().getSmart().smartEnabled && newPowerState;
            if (smartEnabled.get() != newSmartEnabled)
            {
                smartEnabled.set(newSmartEnabled);
                writePropertiesToKernelFlag = true;
            }
        }

        if (future)
        {
            if (future->valid() && future->wait_for(std::chrono::seconds(0)) ==
                                       std::future_status::ready)
            {
                const auto& status = future->get();
                if (status != std::ios_base::goodbit)
                {
                    Logger::log<LogLevel::error>(
                        "[SmartSupervisor] Writing properties to kernel has "
                        "failed.");
                }
                else
                {
                    Logger::log<LogLevel::info>(
                        "[SmartSupervisor] Properties saved to kernel.");
                }
            }
        }
        if (writePropertiesToKernelFlag)
        {
            if (!future || !future->valid())
            {
                if (isKernelModule())
                {
                    Logger::log<LogLevel::info>(
                        "[SmartSupervisor] Writing properties to kernel "
                        "triggered...");
                    writePropertiesToKernelFlag = false;
                    auto params = std::make_tuple(
                        std::make_tuple(psuPollingIntervalMs.get(),
                                        "smart_psu_polling_interval_time"),
                        std::make_tuple(overtemperatureThrottlingTimeMs.get(),
                                        "overtemperature_throttling_time"),
                        std::make_tuple(overcurrentThrottlingTimeMs.get(),
                                        "overcurrent_throttling_time"),
                        std::make_tuple(undervoltageThrottlingTimeMs.get(),
                                        "undervoltage_throttling_time"),
                        std::make_tuple(maxUndervoltageTimeTimeMs.get(),
                                        "max_undervoltage_time"),
                        std::make_tuple(maxOvertemperatureTimeMs.get(),
                                        "max_overtemperature_time"),
                        std::make_tuple(
                            powergoodPollingIntervalTimeMs.get(),
                            "smart_powergood_polling_interval_time"),
                        std::make_tuple(i2cAddrMax.get(), "i2c_addr_max"),
                        std::make_tuple(i2cAddrMin.get(), "i2c_addr_min"),
                        std::make_tuple(forceSmbalertMaskIntervalTimeMs.get(),
                                        "force_smbalert_mask_interval_time"),
                        std::make_tuple(redundancyEnabled.get(), "redundancy"),
                        std::make_tuple(smartEnabled.get(), "enable"));

                    future = std::move(std::async(
                        std::launch::async,
                        [dir{parametersDir},
                         valuesToSave{params}]() -> std::ios_base::iostate {
                            std::ios_base::iostate status =
                                std::ios_base::goodbit;
                            for_each(valuesToSave,
                                     [&status, dir](auto value, auto filename) {
                                         status |= writePropertyToKernel(
                                             value, dir + "/" + filename);
                                     });
                            return status;
                        }));
                }
            }
        }
        if (writePropertiesToConfigFlag)
        {
            writePropertiesToConfigFlag = false;
            if (!writePropertiesToConfig())
            {
                Logger::log<LogLevel::error>(
                    "[SmartSupervisor] Writing properties to config has "
                    "failed.");
            }
        }
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string const objectPath;
    DbusInterfaces dbusInterfaces;
    std::shared_ptr<SensorReadingsManagerIf> sensorsManager;
    const std::string parametersDir;
    bool writePropertiesToKernelFlag = true;
    bool writePropertiesToConfigFlag = false;
    bool prevPowerStateOn = false;
    std::optional<std::future<std::ios_base::iostate>> future;

    PropertyWrapper<bool> smartEnabled;
    PropertyWrapper<bool> redundancyEnabled;
    PropertyWrapper<uint32_t> psuPollingIntervalMs;
    PropertyWrapper<uint32_t> overtemperatureThrottlingTimeMs;
    PropertyWrapper<uint32_t> overcurrentThrottlingTimeMs;
    PropertyWrapper<uint32_t> undervoltageThrottlingTimeMs;
    PropertyWrapper<uint32_t> maxUndervoltageTimeTimeMs;
    PropertyWrapper<uint32_t> maxOvertemperatureTimeMs;
    PropertyWrapper<uint32_t> powergoodPollingIntervalTimeMs;
    PropertyWrapper<uint32_t> i2cAddrMax;
    PropertyWrapper<uint32_t> i2cAddrMin;
    PropertyWrapper<uint32_t> forceSmbalertMaskIntervalTimeMs;
    PropertyWrapper<bool> kernelModulePresent;

    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            kSmartDbusInterfaceName, [this](auto& smartIfc) {
                smartEnabled.registerProperty(
                    smartIfc, "SmartEnabled",
                    sdbusplus::asio::PropertyPermission::readOnly);
                kernelModulePresent.registerProperty(
                    smartIfc, "KernelModulePresent",
                    sdbusplus::asio::PropertyPermission::readOnly);
                psuPollingIntervalMs.registerProperty(
                    smartIfc, "PsuPollingIntervalMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                overtemperatureThrottlingTimeMs.registerProperty(
                    smartIfc, "OvertemperatureThrottlingTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                overcurrentThrottlingTimeMs.registerProperty(
                    smartIfc, "OvercurrentThrottlingTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                undervoltageThrottlingTimeMs.registerProperty(
                    smartIfc, "UndervoltageThrottlingTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                maxUndervoltageTimeTimeMs.registerProperty(
                    smartIfc, "MaxUndervoltageTimeTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                maxOvertemperatureTimeMs.registerProperty(
                    smartIfc, "MaxOvertemperatureTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                powergoodPollingIntervalTimeMs.registerProperty(
                    smartIfc, "PowergoodPollingIntervalTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                i2cAddrMax.registerProperty(
                    smartIfc, "I2cAddrMax",
                    sdbusplus::asio::PropertyPermission::readWrite);
                i2cAddrMin.registerProperty(
                    smartIfc, "I2cAddrMin",
                    sdbusplus::asio::PropertyPermission::readWrite);
                forceSmbalertMaskIntervalTimeMs.registerProperty(
                    smartIfc, "ForceSmbalertMaskIntervalTimeMs",
                    sdbusplus::asio::PropertyPermission::readWrite);
                redundancyEnabled.registerProperty(
                    smartIfc, "RedundancyEnabled",
                    sdbusplus::asio::PropertyPermission::readWrite);
            });
    }

    static std::ios_base::iostate
        writePropertyToKernel(const uint32_t& value,
                              const std::string& filepath)
    {
        std::ofstream file(filepath);
        if (file.good())
        {
            file << value;
        }
        if (file.good())
        {
            file.flush();
        }
        if (file.good())
        {
            file.close();
        }
        return file.rdstate();
    }

    bool isKernelModule() const
    {
        return std::filesystem::exists(parametersDir + "/enable");
    }

    bool writePropertiesToConfig()
    {
        Smart config;
        config.psuPollingIntervalMs = psuPollingIntervalMs.get();
        config.overtemperatureThrottlingTimeMs =
            overtemperatureThrottlingTimeMs.get();
        config.overcurrentThrottlingTimeMs = overcurrentThrottlingTimeMs.get();
        config.undervoltageThrottlingTimeMs =
            undervoltageThrottlingTimeMs.get();
        config.maxUndervoltageTimeTimeMs = maxUndervoltageTimeTimeMs.get();
        config.maxOvertemperatureTimeMs = maxOvertemperatureTimeMs.get();
        config.powergoodPollingIntervalTimeMs =
            powergoodPollingIntervalTimeMs.get();
        config.i2cAddrsMax = i2cAddrMax.get();
        config.i2cAddrsMin = i2cAddrMin.get();
        config.forceSmbalertMaskIntervalTimeMs =
            forceSmbalertMaskIntervalTimeMs.get();
        config.redundancyEnabled = redundancyEnabled.get();
        config.smartEnabled = Config::getInstance().getSmart().smartEnabled;
        return Config::getInstance().update(config);
    }

    void setDefaultPropertiesValues()
    {
        const Smart& config = Config::getInstance().getSmart();
        psuPollingIntervalMs.set(config.psuPollingIntervalMs);
        overtemperatureThrottlingTimeMs.set(
            config.overtemperatureThrottlingTimeMs);
        overcurrentThrottlingTimeMs.set(config.overcurrentThrottlingTimeMs);
        undervoltageThrottlingTimeMs.set(config.undervoltageThrottlingTimeMs);
        maxUndervoltageTimeTimeMs.set(config.maxUndervoltageTimeTimeMs);
        maxOvertemperatureTimeMs.set(config.maxOvertemperatureTimeMs);
        powergoodPollingIntervalTimeMs.set(
            config.powergoodPollingIntervalTimeMs);
        i2cAddrMax.set(config.i2cAddrsMax);
        i2cAddrMin.set(config.i2cAddrsMin);
        forceSmbalertMaskIntervalTimeMs.set(
            config.forceSmbalertMaskIntervalTimeMs);

        redundancyEnabled.set(config.redundancyEnabled);
        smartEnabled.set(config.smartEnabled);
    }

    template <class T>
    auto checkRange(T min, T max) const
    {
        return [min, max](const T& value) -> void {
            verifyRange<errors::OutOfRange>(value, min, max);
        };
    }

    void setVerifyDbusSetFunctions()
    {
        psuPollingIntervalMs.setVerifyDbusSetFunction(checkRange(10, 10000));
        overtemperatureThrottlingTimeMs.setVerifyDbusSetFunction(
            checkRange(100, 10000));
        overcurrentThrottlingTimeMs.setVerifyDbusSetFunction(
            checkRange(100, 10000));
        undervoltageThrottlingTimeMs.setVerifyDbusSetFunction(
            checkRange(100, 10000));
        maxUndervoltageTimeTimeMs.setVerifyDbusSetFunction(
            checkRange(100, 2000));
        maxOvertemperatureTimeMs.setVerifyDbusSetFunction(
            checkRange(100, 2000));
        powergoodPollingIntervalTimeMs.setVerifyDbusSetFunction(
            checkRange(10, 10000));
        i2cAddrMax.setVerifyDbusSetFunction(checkRange(0, 255));
        i2cAddrMin.setVerifyDbusSetFunction(checkRange(0, 255));
        forceSmbalertMaskIntervalTimeMs.setVerifyDbusSetFunction(
            checkRange(1000, 1000000));
    }

    void setPostDbusSetFunctions()
    {
        const auto triggerWrite = [this]() {
            writePropertiesToKernelFlag = true;
            writePropertiesToConfigFlag = true;
        };
        redundancyEnabled.setPostDbusSetFunction(triggerWrite);
        psuPollingIntervalMs.setPostDbusSetFunction(triggerWrite);
        overtemperatureThrottlingTimeMs.setPostDbusSetFunction(triggerWrite);
        overcurrentThrottlingTimeMs.setPostDbusSetFunction(triggerWrite);
        undervoltageThrottlingTimeMs.setPostDbusSetFunction(triggerWrite);
        maxUndervoltageTimeTimeMs.setPostDbusSetFunction(triggerWrite);
        maxOvertemperatureTimeMs.setPostDbusSetFunction(triggerWrite);
        powergoodPollingIntervalTimeMs.setPostDbusSetFunction(triggerWrite);
        i2cAddrMax.setPostDbusSetFunction(triggerWrite);
        i2cAddrMin.setPostDbusSetFunction(triggerWrite);
        forceSmbalertMaskIntervalTimeMs.setPostDbusSetFunction(triggerWrite);
    }
};

} // namespace nodemanager
