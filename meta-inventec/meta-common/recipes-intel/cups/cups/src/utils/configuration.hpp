/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2020 Intel Corporation
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you (License). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or
 *  transmit this software or the related documents without
 *  Intel's prior written permission.
 *
 *  This software and the related documents are provided as is,
 *  with no express or implied warranties, other than those
 *  that are expressly stated in the License.
 */
#pragma once

#include "base/loadFactors.hpp"
#include "utils/log.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <functional>
#include <regex>

namespace cups
{

namespace utils
{

class EntityManager
{
  public:
    using BasicVariantType =
        std::variant<std::vector<std::string>, std::string, int64_t, uint64_t,
                     double, int32_t, uint32_t, int16_t, uint16_t, uint8_t,
                     bool>;
    using SensorBaseConfigMap =
        boost::container::flat_map<std::string, BasicVariantType>;

    using ObjectPath = std::string;
    using ServiceName = std::string;
    using Ifaces = std::vector<std::string>;
    using ObjectIfaces = std::vector<std::pair<ServiceName, Ifaces>>;
    using ServiceIface = std::pair<ObjectPath, ObjectIfaces>;
    using ServiceIfaces = std::vector<ServiceIface>;

    struct SensorConfiguration
    {
        const std::string SensorPath;
        const std::string ConfigName;
        const SensorBaseConfigMap Values;
    };

    static void getSensorConfiguration(
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::string& name, const std::string& config,
        std::function<void(const boost::system::error_code ec,
                           const SensorConfiguration& config)>&& callback)
    {
        boost::asio::spawn(
            bus->get_io_context(),
            [bus{bus}, name{name}, config{config},
             callback{std::move(callback)}](boost::asio::yield_context yield) {
                const auto notFoundError = boost::system::errc::make_error_code(
                    boost::system::errc::not_connected);

                const std::array<std::string, 1> interfaces = {config};
                boost::system::error_code ec;

                // Find all applicable configuration entries
                ServiceIfaces tree = bus->yield_method_call<ServiceIfaces>(
                    yield, ec, "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                    "/xyz/openbmc_project/inventory", 0, interfaces);
                if (ec)
                {
                    LOG_ERROR << "GetSubTree failed: " << ec;
                    callback(ec, {});
                    return;
                }

                auto object = findConfig(tree, name);
                if (!object)
                {
                    LOG_ERROR << "Unable to find configuration: " << config
                              << " for sensor: " << name;
                    callback(notFoundError, {});
                    return;
                }

                SensorBaseConfigMap values =
                    bus->yield_method_call<SensorBaseConfigMap>(
                        yield, ec, "xyz.openbmc_project.EntityManager", *object,
                        "org.freedesktop.DBus.Properties", "GetAll", config);
                if (ec)
                {
                    LOG_ERROR << "Config retrieval failed: " << ec;
                    callback(ec, {});
                    return;
                }

                callback(ec, {*object, config, std::move(values)});
            });
    }

    static bool
        setProperty(const std::shared_ptr<sdbusplus::asio::connection>& bus,
                    const std::string& path, const std::string& iface,
                    const std::string& property, BasicVariantType value)
    {
        sdbusplus::message::message setProperty = bus->new_method_call(
            "xyz.openbmc_project.EntityManager", path.c_str(),
            "org.freedesktop.DBus.Properties", "Set");
        setProperty.append(iface);
        setProperty.append(property);
        setProperty.append(value);

        try
        {
            sdbusplus::message::message reply = bus->call(setProperty);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            LOG_ERROR << "Unable to set property: " << property
                      << " on iface: " << iface << " for object: " << path
                      << ", " << e.description();
            return false;
        }

        return true;
    }

    template <typename T>
    static std::optional<T> read(const std::string& key,
                                 const SensorBaseConfigMap& properties)
    {
        const auto it = properties.find(key);
        if (it == properties.end())
        {
            LOG_ERROR << "Unable to find property: " << key;
            return {};
        }

        auto val = std::get_if<T>(&it->second);
        if (!val)
        {
            LOG_ERROR << "Unable to get value of property: " << key;
            return {};
        }

        return *val;
    }

  private:
    static std::optional<std::string> findConfig(const ServiceIfaces& tree,
                                                 const std::string& name)
    {
        for (const auto& [object, ifacesMap] : tree)
        {
            if (std::filesystem::path(object).filename() == name)
            {
                for (const auto& [service, ifaces] : ifacesMap)
                {
                    if (service == "xyz.openbmc_project.EntityManager")
                    {
                        LOG_DEBUG << "Found config at path: " << object;
                        return object;
                    }
                }
            }
        }

        return std::nullopt;
    }
};

} // namespace utils

class Configuration
{
    static constexpr const char* inventoryPath =
        "/xyz/openbmc_project/inventory";
    static constexpr const char* cupsSensors = "CUPS";
    static constexpr const char* cupsConfig =
        "xyz.openbmc_project.Configuration.CupsSensor.Polling";

  public:
    template <typename T>
    struct Range
    {
        T min;
        T max;
    };

    struct Values
    {
        const std::string path;
        const uint64_t interval;
        const uint64_t averagingPeriod;
        const std::string loadFactorCfg;
        const base::LoadFactors staticLoadFactors;
    };

    static constexpr Range<std::chrono::milliseconds> intervalRange{
        std::chrono::milliseconds(100), std::chrono::milliseconds(1000)};
    static constexpr Range<std::chrono::milliseconds> averagingPeriodRange{
        std::chrono::milliseconds(1000), std::chrono::milliseconds(10000)};
    static constexpr Range<double> staticLoadFactorRange{0, 100};

    using ConfigurationChange =
        std::function<void(const bool isEnabled, const Values& values)>;

    Configuration(std::shared_ptr<sdbusplus::asio::connection>& busArg) :
        bus(busArg), filterTimer(bus->get_io_context()),
        retryTimer(bus->get_io_context())
    {}

    void loadConfiguration(ConfigurationChange&& callback)
    {
        if (callback == nullptr)
        {
            LOG_ERROR << "Null config callback provided";
            return;
        }

        updateCb = std::move(callback);
        monitor();
        query();
    }

    static bool setInterval(std::shared_ptr<sdbusplus::asio::connection> bus,
                            const std::string& path, uint64_t value)
    {
        std::chrono::milliseconds newValue(value);
        if (newValue < intervalRange.min || newValue > intervalRange.max)
        {
            LOG_ERROR << "Invalid interval: " << value;
            return false;
        }

        return utils::EntityManager::setProperty(
            bus, path, cupsConfig, "Interval", static_cast<double>(value));
    }

    static bool
        setAveragingPeriod(std::shared_ptr<sdbusplus::asio::connection> bus,
                           const std::string& path, uint64_t value)
    {
        std::chrono::milliseconds newValue(value);
        if (newValue < averagingPeriodRange.min ||
            newValue > averagingPeriodRange.max)
        {
            LOG_ERROR << "Invalid AveragingPeriod: " << value;
            return false;
        }

        return utils::EntityManager::setProperty(bus, path, cupsConfig,
                                                 "AveragingPeriod",
                                                 static_cast<double>(value));
    }

    static bool
        setLoadFactorCfg(std::shared_ptr<sdbusplus::asio::connection> bus,
                         const std::string& path, std::string value)
    {
        if (!base::validateLoadFactorCfg(value))
        {
            LOG_ERROR << "Invalid Load Factor Configuration " << value;
            return false;
        }

        return utils::EntityManager::setProperty(
            bus, path, cupsConfig, "LoadFactorConfiguration", value);
    }

    static bool validateStaticLoadFactors(double coreLoadFactor,
                                          double iioLoadFactor,
                                          double memoryLoadFactor)
    {
        if (coreLoadFactor < staticLoadFactorRange.min ||
            coreLoadFactor > staticLoadFactorRange.max)
        {
            LOG_ERROR << "Invalid Static Core Load Factor " << coreLoadFactor;
            return false;
        }

        if (iioLoadFactor < staticLoadFactorRange.min ||
            iioLoadFactor > staticLoadFactorRange.max)
        {
            LOG_ERROR << "Invalid Static Iio Load Factor " << iioLoadFactor;
            return false;
        }

        if (memoryLoadFactor < staticLoadFactorRange.min ||
            memoryLoadFactor > staticLoadFactorRange.max)
        {
            LOG_ERROR << "Invalid Static Memory Load Factor "
                      << memoryLoadFactor;
            return false;
        }

        if ((coreLoadFactor + iioLoadFactor + memoryLoadFactor) !=
            staticLoadFactorRange.max)
        {
            LOG_ERROR << "Invalid Static Load Factors Configuration";
            return false;
        }

        return true;
    }

    static bool
        setStaticLoadFactors(std::shared_ptr<sdbusplus::asio::connection> bus,
                             const std::string& path,
                             const std::tuple<double, double, double>& values)
    {
        auto [coreLoadFactor, iioLoadFactor, memoryLoadFactor] = values;

        if (!validateStaticLoadFactors(coreLoadFactor, iioLoadFactor,
                                       memoryLoadFactor))
        {
            return false;
        }

        if (!utils::EntityManager::setProperty(
                bus, path, cupsConfig, "CoreLoadFactor", coreLoadFactor))
        {
            return false;
        }

        if (!utils::EntityManager::setProperty(bus, path, cupsConfig,
                                               "IioLoadFactor", iioLoadFactor))
        {
            return false;
        }

        if (!utils::EntityManager::setProperty(
                bus, path, cupsConfig, "MemoryLoadFactor", memoryLoadFactor))
        {
            return false;
        }

        return true;
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    ConfigurationChange updateCb;
    std::unique_ptr<sdbusplus::bus::match::match> match;
    boost::asio::deadline_timer filterTimer;
    boost::asio::deadline_timer retryTimer;

    void query()
    {
        if (!updateCb)
        {
            LOG_ERROR << "No calback specified for configuration updates";
            return;
        }

        LOG_INFO << "Querying EntityManager for " << cupsConfig;

        utils::EntityManager::getSensorConfiguration(
            bus, cupsSensors, cupsConfig,
            [this](const boost::system::error_code ec,
                   const utils::EntityManager::SensorConfiguration& config) {
                if (ec == boost::system::errc::timed_out)
                {
                    LOG_DEBUG << "EntityManager call timed out, probably due "
                                 "to high system load. Will try again";
                    delayedQuery(retryTimer, boost::posix_time::seconds(10));
                    return;
                }
                else if (ec)
                {
                    LOG_ERROR << "Unable to retrieve CUPS Sensor "
                                 "configuration: "
                              << ec;
                    updateCb(false, {});
                    return;
                }

                const auto& [path, configName, properties] = config;
                LOG_DEBUG << "Sensor path: " << path;
                LOG_DEBUG << "Config entry: " << configName;
                for (const auto& [k, v] : properties)
                {
                    LOG_DEBUG << "Found property: " << k;
                }

                auto interval =
                    utils::EntityManager::read<double>("Interval", properties);
                auto averagingPeriod = utils::EntityManager::read<double>(
                    "AveragingPeriod", properties);
                auto loadFactorCfg = utils::EntityManager::read<std::string>(
                    "LoadFactorConfiguration", properties);
                auto staticCoreLoadFactor = utils::EntityManager::read<double>(
                    "CoreLoadFactor", properties);
                auto staticMemoryLoadFactor =
                    utils::EntityManager::read<double>("MemoryLoadFactor",
                                                       properties);
                auto staticIioLoadFactor = utils::EntityManager::read<double>(
                    "IioLoadFactor", properties);

                if (!interval || !averagingPeriod || !loadFactorCfg ||
                    !staticCoreLoadFactor || !staticIioLoadFactor ||
                    !staticMemoryLoadFactor)
                {
                    updateCb(false, {});
                    return;
                }

                if (!validateStaticLoadFactors(*staticCoreLoadFactor,
                                               *staticIioLoadFactor,
                                               *staticMemoryLoadFactor))
                {
                    // restore default values for static load factors
                    *staticCoreLoadFactor = *staticIioLoadFactor =
                        base::LoadFactors::defaultLoadFactor;
                    *staticMemoryLoadFactor =
                        base::LoadFactors::defaultLoadFactorComplement;
                }

                normalizeInterval(interval);
                normalizeAveragingPeriod(averagingPeriod);

                updateCb(true,
                         {path,
                          static_cast<uint64_t>(*interval),
                          static_cast<uint64_t>(*averagingPeriod),
                          static_cast<std::string>(*loadFactorCfg),
                          {static_cast<double>(*staticCoreLoadFactor),
                           static_cast<double>(*staticIioLoadFactor),
                           static_cast<double>(*staticMemoryLoadFactor)}});
            });
    }

    static void normalizeInterval(std::optional<double>& interval)
    {
        if (interval)
        {
            std::chrono::milliseconds newInterval(
                static_cast<uint64_t>(*interval));
            if (newInterval < intervalRange.min ||
                newInterval > intervalRange.max)
            {
                LOG_ERROR << "Invalid Interval: " << *interval
                          << ", reverting to default: "
                          << intervalRange.max.count();
                *interval = static_cast<double>(intervalRange.max.count());
            }
        }
    }

    static void normalizeAveragingPeriod(std::optional<double>& period)
    {
        if (period)
        {
            std::chrono::milliseconds newPeriod(static_cast<uint64_t>(*period));
            if (newPeriod < averagingPeriodRange.min ||
                newPeriod > averagingPeriodRange.max)
            {
                LOG_ERROR << "Invalid AveragingPeriod: " << *period
                          << ", reverting to default: "
                          << averagingPeriodRange.max.count();
                *period = static_cast<double>(averagingPeriodRange.max.count());
            }
        }
    }

    void monitor()
    {
        if (!match)
        {
            std::string query = "type='signal',member='PropertiesChanged',"
                                "path_namespace='" +
                                std::string(inventoryPath) +
                                "',arg0namespace='" + cupsConfig + "'";

            match = std::make_unique<typeof(*match)>(
                static_cast<sdbusplus::bus::bus&>(*bus), query,
                std::bind(&Configuration::configChange, this,
                          std::placeholders::_1));
        }
    }

    void configChange(sdbusplus::message::message& m)
    {
        static const std::regex pattern(std::string(inventoryPath) + ".*/" +
                                        cupsSensors);

        LOG_DEBUG << "Signal from path: " << m.get_path();
        if (std::regex_match(m.get_path(), pattern))
        {
            /**
             * PropertiesChanged events have tendency to flood the
             * recipient.
             *
             * Wait some time after signal is received, so multiple events
             * can be handled in single shot.
             */
            delayedQuery(filterTimer, boost::posix_time::seconds(1));
        }
    }

    void delayedQuery(boost::asio::deadline_timer& timer,
                      boost::posix_time::seconds delay)
    {
        // this implicitly aborts current timer execution
        timer.expires_from_now(delay);
        timer.async_wait([&](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }
            else if (ec)
            {
                LOG_ERROR << "Timer error: " << ec;
                return;
            }

            query();
        });
    }
};

} // namespace cups
