/*
 * INTEL CONFIDENTIAL
 *
 * Copyright2022 Intel Corporation.
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

#include <boost/asio.hpp>
#include <map>
#include <sdbusplus/asio/property.hpp>

namespace nodemanager
{

struct PldmEntityData
{
    std::string tid;
    std::string deviceName;

    bool operator==(const PldmEntityData& rhs) const
    {
        return tid == rhs.tid && deviceName == rhs.deviceName;
    }
};

class PldmEntityProviderIf
{
  public:
    virtual ~PldmEntityProviderIf() = default;
    virtual std::optional<std::string>
        getTid(DeviceIndex componentId) const = 0;
    virtual std::optional<std::string>
        getDeviceName(DeviceIndex componentId) const = 0;
    virtual void registerDiscoveryDataChangeCallback(
        const std::shared_ptr<std::function<void(void)>>&) = 0;
    virtual void unregisterDiscoveryDataChangeCallback(
        const std::shared_ptr<std::function<void(void)>>&) = 0;
};

/**
 * @brief This class provides information about discovered pldm entities
 *
 */
class PldmEntityProvider : public PldmEntityProviderIf
{
  public:
    PldmEntityProvider() = delete;
    PldmEntityProvider(const PldmEntityProvider&) = delete;
    PldmEntityProvider& operator=(const PldmEntityProvider&) = delete;
    PldmEntityProvider(PldmEntityProvider&&) = delete;
    PldmEntityProvider& operator=(PldmEntityProvider&&) = delete;

    PldmEntityProvider(std::shared_ptr<sdbusplus::asio::connection> busArg) :
        bus(busArg), discoveryTimer(busArg->get_io_context()),
        discoveryTimeout(std::chrono::seconds(1))
    {
        runDiscovery();
    }

    virtual ~PldmEntityProvider() = default;

    std::optional<std::string> getTid(DeviceIndex componentId) const final
    {
        auto search = pldmEntities.find(componentId);
        if (search != pldmEntities.end())
        {
            return search->second.tid;
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<std::string>
        getDeviceName(DeviceIndex componentId) const final
    {
        auto search = pldmEntities.find(componentId);
        if (search != pldmEntities.end())
        {
            return search->second.deviceName;
        }
        else
        {
            return std::nullopt;
        }
    }

    void registerDiscoveryDataChangeCallback(
        const std::shared_ptr<std::function<void(void)>>& callbackArg)
    {
        discoveryDataChangeCallbacks.push_back(callbackArg);
    }

    virtual void unregisterDiscoveryDataChangeCallback(
        const std::shared_ptr<std::function<void(void)>>& callbackArg)
    {
        const auto& it =
            std::find(discoveryDataChangeCallbacks.begin(),
                      discoveryDataChangeCallbacks.end(), callbackArg);
        if (it != discoveryDataChangeCallbacks.end())
        {
            discoveryDataChangeCallbacks.erase(it);
        }
    }

  private:
    using DbusVariantType = std::variant<
        std::vector<std::tuple<std::string, std::string, std::string>>, double,
        uint64_t, uint32_t, uint16_t, uint8_t, bool, std::string,
        std::vector<uint16_t>, std::vector<uint8_t>>;

    using DBusPropertiesMap =
        std::vector<std::pair<std::string, DbusVariantType>>;
    using DBusInterfacesMap =
        std::vector<std::pair<std::string, DBusPropertiesMap>>;
    using ManagedObjectType = std::vector<
        std::pair<sdbusplus::message::object_path, DBusInterfacesMap>>;

    static constexpr uint16_t kAddInCardPldmEntityType{68};
    static constexpr std::chrono::seconds kDiscoveryPeriod{10};

    std::shared_ptr<sdbusplus::asio::connection> bus;
    boost::asio::steady_timer discoveryTimer;
    std::chrono::seconds discoveryTimeout;

    std::unordered_map<DeviceIndex, PldmEntityData> pldmEntities;
    std::unordered_map<DeviceIndex, PldmEntityData> newPldmEntities;

    std::vector<std::shared_ptr<std::function<void(void)>>>
        discoveryDataChangeCallbacks;

    void clearNewPldmEntities()
    {
        newPldmEntities.clear();
    }

    void storePldmEntities()
    {
        if (newPldmEntities != pldmEntities)
        {
            pldmEntities = newPldmEntities;

            for (const auto& callback : discoveryDataChangeCallbacks)
            {
                (*callback)();
            }
        }
    }

    void addNewPldmEntity(uint16_t instanceNumber, std::string path)
    {
        if (instanceNumber == 0 || instanceNumber > kMaxPcieNumber)
        {
            Logger::log<LogLevel::error>("[PldmEntityProvider]: Invalid PLDM "
                                         "entity %s instance number %d",
                                         path, instanceNumber);
            return;
        }

        DeviceIndex componentId{static_cast<DeviceIndex>(instanceNumber - 1)};

        sdbusplus::message::object_path dbusPath(path);
        PldmEntityData entityData{};
        entityData.tid = dbusPath.parent_path().filename();
        entityData.deviceName = dbusPath.filename();

        newPldmEntities.insert_or_assign(componentId, entityData);
    }

    void discoverPldmEntities()
    {
        bus->async_method_call(
            [this](const boost::system::error_code& err,
                   ManagedObjectType& managedObj) {
                clearNewPldmEntities();

                if (err)
                {
                    Logger::log<LogLevel::debug>(
                        "[PldmEntityProvider]: Error while trying to "
                        "GetManagedObjects on PLDM, "
                        "err=%s\n",
                        err.message());
                    return;
                }

                for (auto& [path, ifMap] : managedObj)
                {
                    if (auto entityPropMap = std::ranges::find(
                            ifMap, "xyz.openbmc_project.PLDM.Entity",
                            &std::pair<std::string, DBusPropertiesMap>::first);
                        entityPropMap != ifMap.end())
                    {
                        bool entityFound{false};
                        uint16_t instanceNumber{
                            std::numeric_limits<uint16_t>::quiet_NaN()};

                        for (auto& [propName, variantValue] :
                             entityPropMap->second)
                        {
                            if (propName == "EntityType")
                            {
                                if (auto value =
                                        std::get_if<uint16_t>(&variantValue))
                                {
                                    if (*value == kAddInCardPldmEntityType)
                                    {
                                        entityFound = true;
                                    }
                                }
                                else
                                {
                                    Logger::log<LogLevel::error>(
                                        "[PldmEntityProvider]: EntityType "
                                        "value type different"
                                        "than expected");
                                }
                            }
                            else if (propName == "EntityInstanceNumber")
                            {
                                if (auto value =
                                        std::get_if<uint16_t>(&variantValue))
                                {
                                    instanceNumber = *value;
                                }
                                else
                                {
                                    Logger::log<LogLevel::error>(
                                        "[PldmEntityProvider]: "
                                        "EntityInstanceNumber "
                                        "value type different than "
                                        "expected");
                                }
                            }

                            if (entityFound &&
                                instanceNumber !=
                                    std::numeric_limits<uint16_t>::quiet_NaN())
                            {
                                addNewPldmEntity(instanceNumber, path);
                                break;
                            }
                        }
                    }
                }

                storePldmEntities();
            },
            "xyz.openbmc_project.pldm", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void runDiscovery()
    {
        discoverPldmEntities();

        discoveryTimer.expires_after(discoveryTimeout);
        discoveryTimer.async_wait([this](boost::system::error_code ec) {
            if (ec)
            {
                Logger::log<LogLevel::warning>(
                    "[PldmEntityProvider]: Unexpected discovery timer"
                    "cancellation. You should probably restart NodeManager"
                    "service. Error code: %d",
                    ec);
                return;
            }
            discoveryTimeout = kDiscoveryPeriod;
            runDiscovery();
        });
    }
}; // class PldmEntityProvider

} // namespace nodemanager
