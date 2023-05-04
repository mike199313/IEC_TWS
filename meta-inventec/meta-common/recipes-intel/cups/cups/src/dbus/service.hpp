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

#include "base/sensor.hpp"
#include "base/service.hpp"
#include "dbus/dbus.hpp"
#include "dbus/sensor.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <filesystem>
#include <memory>

namespace cups
{

namespace dbus
{

class CupsService
{
    // Prevents constructor from being called externally
    struct ctor_lock
    {};

  public:
    CupsService(ctor_lock, std::shared_ptr<sdbusplus::asio::connection> busArg,
                std::shared_ptr<sdbusplus::asio::object_server> objServerArg,
                const std::string& configPathArg,
                std::shared_ptr<peci::transport::Adapter> peciAdapterArg) :
        bus(busArg),
        objServer(objServerArg),
        cupsService(base::CupsService::make(bus->get_io_context(), bus,
                                            peciAdapterArg)),
        configPath(configPathArg)
    {
        createConfigurationInterface();
        createDynamicLoadFactorsInterface();
        createStaticLoadFactorsInterface();
    }

    static std::shared_ptr<CupsService>
        make(std::shared_ptr<sdbusplus::asio::connection> bus,
             std::shared_ptr<sdbusplus::asio::object_server> objServer,
             const std::string& configPath,
             std::shared_ptr<peci::transport::Adapter> peciAdapter)
    {
        auto srv = std::make_shared<CupsService>(ctor_lock{}, bus, objServer,
                                                 configPath, peciAdapter);

        std::string chassis =
            std::filesystem::path(configPath).parent_path().string();

        for (auto& sensor : srv->cupsService->getSensors())
        {
            srv->sensors.emplace_back(std::make_shared<dbus::Sensor>(
                srv->bus, srv->objServer, sensor, chassis));
        }

        return srv;
    }

    std::chrono::milliseconds getInterval()
    {
        return cupsService->getInterval();
    }

    void setInterval(std::chrono::milliseconds interval)
    {
        LOG_DEBUG << "Setting interval to " << interval.count() << " ms";
        cfgIface->set_property("Interval",
                               static_cast<uint64_t>(interval.count()));
        cupsService->setInterval(std::chrono::milliseconds(interval));
    }

    std::chrono::milliseconds getAveragingPeriod()
    {
        return cupsService->getAveragingPeriod();
    }

    void setAveragingPeriod(std::chrono::milliseconds averagingPeriod)
    {
        LOG_DEBUG << "Setting averagingPeriod to " << averagingPeriod.count()
                  << " ms";
        cfgIface->set_property("AveragingPeriod",
                               static_cast<uint64_t>(averagingPeriod.count()));
        cupsService->setAveragingPeriod(
            std::chrono::milliseconds(averagingPeriod));
    }

    const std::string& getLoadFactorCfg() const
    {
        return base::fromLoadFactorCfg(cupsService->getLoadFactorCfg());
    }

    void setLoadFactorCfg(const std::string& loadFactorCfgStr)
    {
        LOG_DEBUG << "Setting loadFactorCfg to " << loadFactorCfgStr;

        std::optional<base::LoadFactorCfg> loadFactorCfg =
            base::toLoadFactorCfg(loadFactorCfgStr);
        if (loadFactorCfg)
        {
            cfgIface->set_property("LoadFactorConfiguration", loadFactorCfgStr);
            cupsService->setLoadFactorCfg(*loadFactorCfg);
        }
    }

    const base::LoadFactors& getStaticLoadFactors() const
    {
        return cupsService->getStaticLoadFactors();
    }

    std::tuple<double, double, double> getStaticLoadFactorsTuple() const
    {
        return base::toTuple(cupsService->getStaticLoadFactors());
    }

    void setStaticLoadFactors(const std::tuple<double, double, double>& values)
    {
        auto [coreLoadFactor, iioLoadFactor, memoryLoadFactor] = values;
        LOG_DEBUG << "Setting static coreLoadFactor to " << coreLoadFactor
                  << " %, memory to " << memoryLoadFactor << " %, iio to "
                  << iioLoadFactor << " %";

        if (staticLoadFactorsIface->set_property("StaticLoadFactors", values))
        {
            cupsService->setStaticLoadFactors(coreLoadFactor, iioLoadFactor,
                                              memoryLoadFactor);
        }
        else
        {
            LOG_ERROR << "Failed to set static coreLoadFactor to "
                      << coreLoadFactor << " %, memory to " << memoryLoadFactor
                      << " %, iio to " << iioLoadFactor
                      << " %. Restoring defaults";

            // set defaults
            base::LoadFactors loadFactorsDefaults;
            staticLoadFactorsIface->set_property(
                "StaticLoadFactors", base::toTuple(loadFactorsDefaults));

            cupsService->setStaticLoadFactors(
                loadFactorsDefaults.coreLoadFactor,
                loadFactorsDefaults.iioLoadFactor,
                loadFactorsDefaults.memoryLoadFactor);
        }
    }

    ~CupsService()
    {
        objServer->remove_interface(cfgIface);
        objServer->remove_interface(dynamicLoadFactorsIface);
        objServer->remove_interface(staticLoadFactorsIface);
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> cfgIface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> dynamicLoadFactorsIface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> staticLoadFactorsIface;
    std::vector<std::shared_ptr<dbus::Sensor>> sensors;

    std::shared_ptr<base::CupsService> cupsService;
    std::string configPath;

    CupsService(const CupsService&) = delete;
    CupsService& operator=(const CupsService&) = delete;

    void createConfigurationInterface()
    {
        cfgIface = objServer->add_interface(dbus::Path,
                                            dbus::subIface("Configuration"));

        cfgIface->register_property(
            "Interval",
            static_cast<uint64_t>(cupsService->getInterval().count()),
            [bus(bus), configPath(configPath), cupsService(cupsService)](
                const uint64_t newValue, uint64_t& value) {
                if (Configuration::setInterval(bus, configPath, newValue))
                {
                    cupsService->setInterval(
                        std::chrono::milliseconds(newValue));
                    value = newValue;
                    return true;
                }
                throw sdbusplus::exception::SdBusError(
                    EINVAL, "Invalid Interval value");
            });

        cfgIface->register_property(
            "AveragingPeriod",
            static_cast<uint64_t>(cupsService->getAveragingPeriod().count()),
            [bus(bus), configPath(configPath), cupsService(cupsService)](
                const uint64_t newValue, uint64_t& value) {
                if (Configuration::setAveragingPeriod(bus, configPath,
                                                      newValue))
                {
                    cupsService->setAveragingPeriod(
                        std::chrono::milliseconds(newValue));
                    value = newValue;
                    return true;
                }
                throw sdbusplus::exception::SdBusError(
                    EINVAL, "Invalid AveragingPeriod value");
            });

        cfgIface->register_property(
            "LoadFactorConfiguration",
            base::fromLoadFactorCfg(cupsService->getLoadFactorCfg()),
            [bus(bus), configPath(configPath), cupsService(cupsService)](
                const std::string& newValue, std::string& value) -> bool {
                if (Configuration::setLoadFactorCfg(bus, configPath, newValue))
                {
                    std::optional<base::LoadFactorCfg> loadFactorCfg =
                        base::toLoadFactorCfg(newValue);
                    if (loadFactorCfg)
                    {
                        cupsService->setLoadFactorCfg(*loadFactorCfg);

                        value = newValue;
                        return true;
                    }
                }
                throw sdbusplus::exception::SdBusError(
                    EINVAL, "Invalid LoadFactorConfiguration value");
            },
            [cupsService(cupsService)](const std::string& value) {
                return base::fromLoadFactorCfg(cupsService->getLoadFactorCfg());
            });

        cfgIface->initialize();
    }

    void createDynamicLoadFactorsInterface()
    {
        dynamicLoadFactorsIface =
            objServer->add_interface(dbus::subPath("DynamicLoadFactors"),
                                     dbus::subIface("DynamicLoadFactors"));

        dynamicLoadFactorsIface->register_property_r(
            "DynamicLoadFactors",
            base::toTuple(cupsService->getDynamicLoadFactors()),
            sdbusplus::vtable::property_::const_,
            [cupsService(cupsService)](
                std::tuple<double, double, double>& values) {
                return base::toTuple(cupsService->getDynamicLoadFactors());
            });

        dynamicLoadFactorsIface->initialize();
    }

    void createStaticLoadFactorsInterface()
    {
        staticLoadFactorsIface =
            objServer->add_interface(dbus::subPath("StaticLoadFactors"),
                                     dbus::subIface("StaticLoadFactors"));

        staticLoadFactorsIface->register_property(
            "StaticLoadFactors",
            base::toTuple(cupsService->getStaticLoadFactors()),
            [bus(bus), configPath(configPath), cupsService(cupsService)](
                const std::tuple<double, double, double>& newValues,
                std::tuple<double, double, double>& values) -> bool {
                if (Configuration::setStaticLoadFactors(bus, configPath,
                                                        newValues))
                {
                    auto [coreLoadFactor, iioLoadFactor, memoryLoadFactor] =
                        newValues;
                    cupsService->setStaticLoadFactors(
                        coreLoadFactor, iioLoadFactor, memoryLoadFactor);
                    values = newValues;
                    return true;
                }
                throw sdbusplus::exception::SdBusError(
                    EINVAL, "Invalid StaticLoadFactors value");
            },
            [cupsService(cupsService)](
                std::tuple<double, double, double>& values) {
                return base::toTuple(cupsService->getStaticLoadFactors());
            });

        staticLoadFactorsIface->initialize();
    }
};

} // namespace dbus

} // namespace cups
