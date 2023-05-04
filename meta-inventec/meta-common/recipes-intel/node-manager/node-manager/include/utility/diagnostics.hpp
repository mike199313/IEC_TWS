/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
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
#include "performance_monitor.hpp"
#include "status_provider_if.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/connection.hpp>

namespace nodemanager
{

static constexpr const char* kOsReleasePath{"/etc/os-release"};
static constexpr const char* kVersionTag{"VERSION_ID="};
static constexpr const char* kOpenbmcVersionTag{"OPENBMC_VERSION="};

class Diagnostics : public StatusProviderIf
{
  private:
    using DbusVariantType =
        std::variant<bool, std::string, int32_t, double, uint32_t, uint16_t,
                     std::vector<uint8_t>, uint8_t>;

    using DBusPropertiesMap =
        boost::container::flat_map<std::string, DbusVariantType>;
    using DBusInteracesMap =
        boost::container::flat_map<std::string, DBusPropertiesMap>;
    using ManagedObjectType = std::vector<
        std::pair<sdbusplus::message::object_path, DBusInteracesMap>>;

  public:
    Diagnostics() = delete;
    Diagnostics(const Diagnostics&) = delete;
    Diagnostics& operator=(const Diagnostics&) = delete;
    Diagnostics(Diagnostics&&) = delete;
    Diagnostics& operator=(Diagnostics&&) = delete;
    Diagnostics(
        std::shared_ptr<sdbusplus::asio::connection> busArg,
        std::string const& objectPathArg,
        std::shared_ptr<DevicesManager> devicesManagerArg,
        std::shared_ptr<ThrottlingLogCollector> throttlingLogCollectorArg) :
        bus(busArg),
        devicesManager(devicesManagerArg),
        throttlingLogCollector(throttlingLogCollectorArg),
        objectServer(std::make_shared<sdbusplus::asio::object_server>(bus)),
        objectPath(objectPathArg + "/Diagnostics"),
        performance(std::make_shared<PerformanceCollector>())
    {
        performanceCollectorWp = performance;
        initializeDbusInterfaces();
    };

    virtual ~Diagnostics() = default;
    void reportStatus(nlohmann::json& out) const final
    {
        // get internal NM status
        if (devicesManager)
        {
            devicesManager->reportStatus(out);
        }

        if (performance)
        {
            out["Performance"]["MeasurementEnabled"] = true;
            performance->reportStatus(out);
        }
        else
        {
            out["Performance"]["MeasurementEnabled"] = false;
        }
    }

    NmHealth getHealth() const final
    {

        std::set<NmHealth> allHealth = {devicesManager->getHealth(),
                                        performance->getHealth()};
        return getMostRestrictiveHealth(allHealth);
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<DevicesManager> devicesManager;
    std::shared_ptr<ThrottlingLogCollector> throttlingLogCollector;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string const objectPath;
    DbusInterfaces dbusInterfaces{objectPath, objectServer};
    std::shared_ptr<PerformanceCollector> performance = nullptr;

    /**
     * @brief Class used to summarize asynchronous calls.
     */
    class AsyncJson
    {
        using Callback = std::function<void(nlohmann::json& out)>;

      public:
        AsyncJson(const Callback callbackArg) : callback(callbackArg)
        {
        }

        ~AsyncJson()
        {
            if (callback)
            {
                callback(out);
            }
        }

        inline nlohmann::json& getJson()
        {
            return out;
        }

      private:
        nlohmann::json out;
        Callback callback;
    };

    void addImageVersion(nlohmann::json& out)
    {
        if (std::filesystem::exists(kOsReleasePath))
        {
            std::ifstream in(kOsReleasePath);
            for (std::string line; getline(in, line);)
            {
                line.erase(
                    std::remove_if(line.begin(), line.end(),
                                   [](const auto& c) { return c == '"'; }),
                    line.end());
                if (line.starts_with(kVersionTag))
                {
                    out["Image"]["Version"]["openbmc-meta-intel"] =
                        line.substr(strlen(kVersionTag));
                }
                else if (line.starts_with(kOpenbmcVersionTag))
                {
                    out["Image"]["Version"]["openbmc-openbmc"] =
                        line.substr(strlen(kOpenbmcVersionTag));
                }
            }
        }
    }

    void addConfiguration(nlohmann::json& out)
    {
        out["Configuration"] = Config::getInstance().toJson();
    }

    void dumpStatusToLog()
    {
        std::shared_ptr<AsyncJson> out =
            std::make_shared<AsyncJson>([](nlohmann::json& jsonOut) {
                Logger::log<LogLevel::info>(jsonOut.dump());
            });
        addTimeStamp(out->getJson());
        reportStatus(out->getJson());
        addImageVersion(out->getJson());
        addConfiguration(out->getJson());

        // get Sx states
        bus->async_method_call(
            [out](const boost::system::error_code& err,
                  std::variant<std::string>& powerState) {
                if (err)
                {
                    Logger::log<LogLevel::info>(
                        "Error while trying to get acpi_power_state, err=%s\n",
                        err.message());
                    out->getJson()["Sx-state"] =
                        "Error while trying to get acpi_power_state, err=" +
                        err.message();
                    return;
                }
                out->getJson()["Sx-state"] = std::get<std::string>(powerState);
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/control/host0/acpi_power_state",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Control.Power.ACPIPowerState",
            "SysACPIStatus");

        // get all NM properties exposed on dbus
        bus->async_method_call(
            [out](const boost::system::error_code& err,
                  ManagedObjectType& managedObj) {
                if (err)
                {
                    Logger::log<LogLevel::info>(
                        "Error while trying to GetManagedObjects on NM, "
                        "err=%s\n",
                        err.message());
                    out->getJson()["NM-properites"] =
                        "Error while trying to GetManagedObjects on NM, err=" +
                        err.message();
                    return;
                }
                for (auto& [path, ifMap] : managedObj)
                {
                    for (auto& [ifName, propMap] : ifMap)
                    {
                        for (auto& [propName, variantValue] : propMap)
                        {
                            std::visit(
                                overloadedHelper{
                                    [out, path, ifName,
                                     propName](double& value) {
                                        if (std::isfinite(value))
                                        {
                                            out->getJson()["NM-properties"]
                                                          [path][ifName]
                                                          [propName] =
                                                std::forward<decltype(value)>(
                                                    value);
                                        }
                                        else
                                        {
                                            out->getJson()["NM-properties"]
                                                          [path][ifName]
                                                          [propName] =
                                                std::to_string(value);
                                        }
                                    },
                                    [out, path, ifName, propName](auto& value) {
                                        out->getJson()["NM-properties"][path]
                                                      [ifName][propName] =
                                            std::forward<decltype(value)>(
                                                value);
                                    }},
                                variantValue);
                        }
                    }
                }
            },
            "xyz.openbmc_project.NodeManager", kRootObjectPath,
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void addTimeStamp(nlohmann::json& out)
    {
        out["Timestamp"] =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    void createPerformanceReport(nlohmann::json& out)
    {
        addTimeStamp(out);
        out["Performance"]["MeasurementEnabled"] = true;
        performance->reportStatus(out);
        addImageVersion(out);
    }

    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.Status", [this](auto& iface) {
                iface.register_method("DumpToLog",
                                      [this]() { dumpStatusToLog(); });
                iface.register_method("DumpToJson", [this]() {
                    nlohmann::json out;
                    addTimeStamp(out);
                    reportStatus(out);
                    addImageVersion(out);
                    addConfiguration(out);
                    return out.dump();
                });
                iface.register_method("GetThrottlingLog", [this]() {
                    return throttlingLogCollector->getJson().dump();
                });
            });

        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.Performance", [this](auto& iface) {
                iface.register_property_rw(
                    "Enabled", bool(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        if (oldValue == newValue)
                        {
                            return 0;
                        }
                        oldValue = newValue;
                        if (newValue)
                        {
                            performance =
                                std::make_shared<PerformanceCollector>();
                            performanceCollectorWp = performance;
                        }
                        else
                        {
                            performance = nullptr;
                            performanceCollectorWp.reset();
                        }
                        return 1;
                    },
                    [this](const auto&) {
                        return static_cast<bool>(performance);
                    });

                iface.register_method("Reset", [this]() {
                    if (performance)
                    {
                        performance->reset();
                        return true;
                    }
                    return false;
                });

                iface.register_method("DumpToJson", [this]() {
                    nlohmann::json out;
                    if (performance)
                    {
                        createPerformanceReport(out);
                    }
                    return out.dump();
                });

                iface.register_method("DumpToLog", [this]() {
                    if (performance)
                    {
                        nlohmann::json out;
                        createPerformanceReport(out);
                        Logger::log<LogLevel::info>(out.dump());
                        return true;
                    }
                    return false;
                });
            });

        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.Logger", [this](auto& iface) {
                iface.register_property_rw(
                    "LogLevel", std::string(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        if (oldValue == newValue)
                        {
                            return 0;
                        }

                        auto logL = strToEnum<errors::InvalidLogLevel>(
                            kLogLevelNames, newValue);

                        Logger::setLogLevel(logL);
                        oldValue = newValue;
                        return 1;
                    },
                    [this](const auto&) {
                        return enumToStr(kLogLevelNames, Logger::getLogLevel());
                    });

                iface.register_property_rw(
                    "RateLimitBurst", std::uint16_t(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        if (oldValue == newValue)
                        {
                            return 0;
                        }

                        Logger::setRateLimitBurst(newValue);
                        oldValue = newValue;
                        return 1;
                    },
                    [this](const auto&) {
                        return Logger::getRateLimitBurst();
                    });

                iface.register_property_rw(
                    "RateLimitIntervalSec", std::int64_t(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, auto& oldValue) {
                        if (oldValue == newValue)
                        {
                            return 0;
                        }

                        Logger::setRateLimitInterval(newValue);
                        oldValue = newValue;
                        return 1;
                    },
                    [this](const auto&) {
                        return Logger::getRateLimitInterval();
                    });
            });
    }
};

} // namespace nodemanager
