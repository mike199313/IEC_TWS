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

#include "common_types.hpp"
#include "loggers/log.hpp"
#include "sensor.hpp"
#include "utility/dbus_common.hpp"
#include "utility/enum_to_string.hpp"
#include "utility/final_callback.hpp"
#include "utility/overloaded_helper.hpp"
#include "utility/performance_monitor.hpp"

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <variant>

namespace nodemanager
{

static constexpr const auto kMapperBusName = "xyz.openbmc_project.ObjectMapper";
static constexpr const auto kMapperPath = "/xyz/openbmc_project/object_mapper";
static constexpr const auto kMapperInterface =
    "xyz.openbmc_project.ObjectMapper";
static constexpr const auto kPropertiesIface =
    "org.freedesktop.DBus.Properties";
template <class T>
class DbusSensor : public Sensor,
                   public std::enable_shared_from_this<DbusSensor<T>>
{
  public:
    DbusSensor() = delete;
    DbusSensor(const DbusSensor&) = delete;
    DbusSensor& operator=(const DbusSensor&) = delete;
    DbusSensor(DbusSensor&&) = delete;
    DbusSensor& operator=(DbusSensor&&) = delete;

    DbusSensor(const std::shared_ptr<SensorReadingsManagerIf>&
                   sensorReadingsManagerArg,
               const std::shared_ptr<sdbusplus::asio::connection>& busArg,
               const std::string& sensorBusNameArg,
               const std::string& sensorValueIfaceArg) :
        Sensor(sensorReadingsManagerArg),
        bus(busArg), sensorBusName(sensorBusNameArg),
        sensorValueIface(sensorValueIfaceArg)
    {

        nameOwnerChangedMatcher =
            std::make_unique<sdbusplus::bus::match::match>(
                static_cast<sdbusplus::bus::bus&>(*bus),
                sdbusplus::bus::match::rules::nameOwnerChanged() +
                    sdbusplus::bus::match::rules::argN(0, sensorBusName) +
                    sdbusplus::bus::match::rules::argN(2, ""),
                nameOwnerChangedHandler);
    }

    std::function<void(sdbusplus::message::message&)> nameOwnerChangedHandler =
        [&](sdbusplus::message::message& message) {
            Logger::log<LogLevel::warning>(
                "Detected service lost from dbus: %s", sensorBusName);
            for (auto& reading : readings)
            {
                reading->setStatus(SensorReadingStatus::unavailable);
            }
        };

    virtual ~DbusSensor() = default;

    virtual void reportStatus(nlohmann::json& out) const override
    {
        for (auto& [sensorReading, path] : objectPathMapping)
        {
            nlohmann::json tmp;
            auto type = enumToStr(kSensorReadingTypeNames,
                                  sensorReading->getSensorReadingType());
            tmp["Status"] =
                enumToStr(sensorReadingStatusNames, sensorReading->getStatus());
            tmp["Health"] = enumToStr(healthNames, sensorReading->getHealth());
            tmp["DeviceIndex"] = sensorReading->getDeviceIndex();
            std::visit([&tmp](auto&& value) { tmp["Value"] = value; },
                       sensorReading->getValue());
            tmp["ObjectPath"] = path;
            out["Sensors-dbus"][type].push_back(tmp);
        }
    }

  protected:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::vector<std::tuple<std::shared_ptr<SensorReadingIf>, std::string>>
        objectPathMapping;
    std::unique_ptr<sdbusplus::bus::match::match> nameOwnerChangedMatcher;

    virtual void dbusInterpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const DBusValue& dbusValue)
    {

        std::visit(overloadedHelper{
                       [this, &sensorReading](T v) {
                           interpretSensorValue(sensorReading, v);
                       },
                       [&sensorReading](auto v) {
                           Logger::log<LogLevel::error>(
                               "Unexpected value from sensor %s-%d\n",
                               enumToStr(kSensorReadingTypeNames,
                                         sensorReading->getSensorReadingType()),
                               unsigned{sensorReading->getDeviceIndex()});
                           sensorReading->setStatus(
                               SensorReadingStatus::invalid);
                       }},
                   dbusValue);
    }

    virtual void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const T& value) = 0;

    const DbusHandlerCallback dbusGetPropertiesHandler(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const std::string& objectPath, int retries, int retryInterval)
    {
        return [weakSelf = this->weak_from_this(), sensorReading, objectPath,
                retries, retryInterval](const boost::system::error_code& err,
                                        const DBusValue& dbusValue) {
            if (err)
            {
                Logger::log<LogLevel::info>(
                    "Sensor %s-%d has not been read, err=%d, "
                    "retries left=%d, retry interval=%d\n",
                    enumToStr(kSensorReadingTypeNames,
                              sensorReading->getSensorReadingType()),
                    unsigned{sensorReading->getDeviceIndex()}, err, retries,
                    retryInterval);
                sensorReading->setStatus(SensorReadingStatus::unavailable);

                if (auto self = weakSelf.lock())
                {
                    if (retries == 0)
                    {
                        return;
                    }
                    auto timer = std::make_shared<boost::asio::steady_timer>(
                        self->bus->get_io_context());
                    timer->expires_after(std::chrono::seconds(retryInterval));
                    timer->async_wait(
                        [weakSelf, sensorReading, objectPath, retries,
                         retryInterval](boost::system::error_code ec) {
                            if (ec)
                            {
                                return;
                            }
                            if (auto selfInTimer = weakSelf.lock())
                            {
                                selfInTimer->bus->async_method_call(
                                    selfInTimer->dbusGetPropertiesHandler(
                                        sensorReading, objectPath, retries - 1,
                                        retryInterval),
                                    selfInTimer->sensorBusName, objectPath,
                                    kPropertiesIface, "Get",
                                    selfInTimer->sensorValueIface,
                                    mapSensorReadingTypeToRequestedValue(
                                        sensorReading->getSensorReadingType()));
                            }
                        });
                }
            }
            else
            {
                Logger::log<LogLevel::info>(
                    "Correct reading value for sensor %s-%d\n",
                    enumToStr(kSensorReadingTypeNames,
                              sensorReading->getSensorReadingType()),
                    unsigned{sensorReading->getDeviceIndex()});
                if (auto self = weakSelf.lock())
                {
                    self->dbusInterpretSensorValue(sensorReading, dbusValue);
                }
            }
        };
    }

    void dbusUpdateDeviceReadings(const DeviceIndex deviceIndex,
                                  int retries = 3, int retryInterval = 1)
    {
        for (auto& tup : objectPathMapping)
        {
            auto&& sensorReading = std::get<0>(tup);
            std::string& objectPath = std::get<1>(tup);

            if (sensorReading->getDeviceIndex() == deviceIndex)
            {
                bus->async_method_call(
                    dbusGetPropertiesHandler(sensorReading, objectPath, retries,
                                             retryInterval),
                    sensorBusName, objectPath, kPropertiesIface, "Get",
                    sensorValueIface,
                    mapSensorReadingTypeToRequestedValue(
                        sensorReading->getSensorReadingType()));
            }
        }
    }

    void dbusRegisterForSensorValueUpdateEvent(void)
    {
        for (auto& tup : objectPathMapping)
        {
            auto&& sensorReading = std::get<0>(tup);
            std::string objectPath = std::get<1>(tup);

            // Register for signal called when sensor is updated
            readingMatches.emplace_back(
                std::make_unique<sdbusplus::bus::match::match>(
                    static_cast<sdbusplus::bus::bus&>(*bus),
                    sdbusplus::bus::match::rules::type::signal() +
                        sdbusplus::bus::match::rules::member(
                            "PropertiesChanged") +
                        sdbusplus::bus::match::rules::path_namespace(
                            objectPath) +
                        sdbusplus::bus::match::rules::arg0namespace(
                            sensorValueIface),
                    dbusSensorPropertiesChangedHandler(sensorReading)));
        };
    }

    void dbusUnregisterSensorValueUpdateEvent()
    {
        readingMatches.clear();
    }

    std::optional<DeviceIndex> getIndexFromPath(const std::string& path) const
    {
        if (std::isdigit(path.back()))
        {
            return path.back() - '1';
        }
        else
        {
            return std::nullopt;
        }
    }

  private:
    const std::string sensorBusName;
    const std::string sensorValueIface;
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> readingMatches;

    const std::function<void(sdbusplus::message::message&)>
        dbusSensorPropertiesChangedHandler(
            std::shared_ptr<nodemanager::SensorReadingIf>& sensorReading)
    {
        return [this, &sensorReading](sdbusplus::message::message& message) {
            std::string iface;
            boost::container::flat_map<std::string, DBusValue>
                changedProperties;
            std::vector<std::string> invalidatedProperties;

            message.read(iface, changedProperties, invalidatedProperties);

            if (iface.compare(sensorValueIface) == 0)
            {
                auto requestedValue = mapSensorReadingTypeToRequestedValue(
                    sensorReading->getSensorReadingType());
                const auto it = changedProperties.find(requestedValue);
                if (it != changedProperties.end())
                {
                    dbusInterpretSensorValue(sensorReading, it->second);
                }
            }
        };
    }

    static std::string
        mapSensorReadingTypeToRequestedValue(const SensorReadingType type)
    {
        switch (type)
        {
            case SensorReadingType::cpuPackagePower:
            case SensorReadingType::dramPower:
            case SensorReadingType::dcPlatformPowerCpu:
            case SensorReadingType::inletTemperature:
            case SensorReadingType::outletTemperature:
            case SensorReadingType::volumetricAirflow:
            case SensorReadingType::cpuEnergy:
            case SensorReadingType::dramEnergy:
            case SensorReadingType::dcPlatformEnergy:
            case SensorReadingType::cpuPackagePowerLimit:
            case SensorReadingType::dramPowerLimit:
            case SensorReadingType::dcPlatformPowerLimit:
            case SensorReadingType::pciePowerPldm:
            case SensorReadingType::pciePowerLimitPldm:
                return "Value";
            case SensorReadingType::cpuPackagePowerCapabilitiesMin:
            case SensorReadingType::pciePowerCapabilitiesMinPldm:
                return "MinValue";
            case SensorReadingType::cpuPackagePowerCapabilitiesMax:
            case SensorReadingType::dramPackagePowerCapabilitiesMax:
            case SensorReadingType::pciePowerCapabilitiesMaxPldm:
                return "MaxValue";
            case SensorReadingType::hostReset:
                return "OperatingSystemState";
            case SensorReadingType::hostPower:
                return "CurrentPowerState";
            case SensorReadingType::powerState:
                return "SysACPIStatus";
            case SensorReadingType::gpuPowerState:
                return "GpuPowerState";
            default:
                throw std::runtime_error(
                    "Unsupported SensorReadingType:" +
                    std::to_string(
                        static_cast<std::underlying_type_t<SensorReadingType>>(
                            type)));
        }
    }
};

} // namespace nodemanager
