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

#include "dbus_sensor.hpp"
#include "devices_manager/pldm_entity_provider.hpp"

namespace nodemanager
{

static constexpr const auto kPcieSensorBusName = "xyz.openbmc_project.pldm";

class PcieDbusSensor : public DbusSensor<double>
{
  public:
    PcieDbusSensor() = delete;
    PcieDbusSensor(const PcieDbusSensor&) = delete;
    PcieDbusSensor& operator=(const PcieDbusSensor&) = delete;
    PcieDbusSensor(PcieDbusSensor&&) = delete;
    PcieDbusSensor& operator=(PcieDbusSensor&&) = delete;

    PcieDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<PldmEntityProviderIf>& pldmEntityProviderArg,
        const std::string& valueInterface) :
        DbusSensor(sensorReadingsManagerArg, busArg, kPcieSensorBusName,
                   valueInterface),
        pldmEntityProvider(pldmEntityProviderArg)
    {
        pldmEntityProvider->registerDiscoveryDataChangeCallback(
            pldmDiscoveryDataChanged);
    }

    virtual ~PcieDbusSensor()
    {
        pldmEntityProvider->unregisterDiscoveryDataChangeCallback(
            pldmDiscoveryDataChanged);
    }

    virtual void initialize() override
    {
        dbusRegisterForSensorValueUpdateEvent();
        for (DeviceIndex index = 0; index < kMaxPcieNumber; ++index)
        {
            dbusUpdateDeviceReadings(index);
        }
    }

  protected:
    void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const double& value) override
    {
        if (!operationalStatuses[sensorReading->getDeviceIndex()])
        {
            sensorReading->setStatus(SensorReadingStatus::unavailable);
        }
        else if (std::isfinite(value))
        {
            sensorReading->updateValue(value);
            sensorReading->setStatus(SensorReadingStatus::valid);
        }
        else
        {
            sensorReading->setStatus(SensorReadingStatus::invalid);
        }
    }

  private:
    std::shared_ptr<PldmEntityProviderIf> pldmEntityProvider;
    std::array<bool, kMaxPcieNumber> operationalStatuses;
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>>
        operationalStatusMatches;

    std::shared_ptr<std::function<void(void)>> pldmDiscoveryDataChanged =
        std::make_shared<std::function<void(void)>>([this]() {
            dbusUnregisterSensorValueUpdateEvent();
            for (const auto& reading : readings)
            {
                sensorReadingsManager->deleteSensorReading(
                    reading->getSensorReadingType());
            }
            readings.clear();
            objectPathMapping.clear();
            operationalStatusMatches.clear();
            std::fill(operationalStatuses.begin(), operationalStatuses.end(),
                      false);
            installSensorReadings();
            initialize();
        });

    virtual std::string getObjectPath(const std::string& tid,
                                      const std::string& device) const = 0;

    virtual std::vector<SensorReadingType> getSensorReadingTypes() const = 0;

    void registerForOperationalStatus(DeviceIndex index,
                                      const std::string& objectPath)
    {
        operationalStatusMatches.push_back(
            std::make_unique<sdbusplus::bus::match::match>(
                static_cast<sdbusplus::bus::bus&>(*bus),
                sdbusplus::bus::match::rules::type::signal() +
                    sdbusplus::bus::match::rules::member("PropertiesChanged") +
                    sdbusplus::bus::match::rules::path_namespace(objectPath) +
                    sdbusplus::bus::match::rules::arg0namespace(
                        kOperationalStatusInterface),
                [this, index](sdbusplus::message::message& message) {
                    std::string iface;
                    boost::container::flat_map<std::string, DBusValue>
                        changedProperties;
                    std::vector<std::string> invalidatedProperties;

                    message.read(iface, changedProperties,
                                 invalidatedProperties);
                    if (iface.compare(kOperationalStatusInterface) == 0)
                    {
                        const auto it =
                            changedProperties.find(kOperationalStatusProperty);
                        if (it != changedProperties.end())
                        {
                            auto value = std::get_if<bool>(&it->second);
                            if (value)
                            {
                                operationalStatuses[index] = *value;
                                updateReadingsAvailability(index, *value);
                            }
                        }
                    }
                }));
    }

    void updateOperationalStatus(DeviceIndex index,
                                 const std::string& objectPath)
    {
        sdbusplus::asio::getProperty<bool>(
            *bus.get(), kPcieSensorBusName, objectPath,
            kOperationalStatusInterface, kOperationalStatusProperty,
            [this, index](boost::system::error_code ec, bool status) {
                if (ec)
                {
                    return;
                }
                operationalStatuses[index] = status;
                updateReadingsAvailability(index, status);
            });
    }

    void installSensorReadings()
    {
        for (DeviceIndex index = 0; index < kMaxPcieNumber; ++index)
        {
            const auto& tid = pldmEntityProvider->getTid(index);
            const auto& device = pldmEntityProvider->getDeviceName(index);
            if (tid && device)
            {
                const auto& objectPath = getObjectPath(*tid, *device);

                registerForOperationalStatus(index, objectPath);
                updateOperationalStatus(index, objectPath);

                for (const auto& sensorReadingType : getSensorReadingTypes())
                {
                    readings.emplace_back(
                        sensorReadingsManager->createSensorReading(
                            sensorReadingType, index));
                    objectPathMapping.emplace_back(
                        std::make_tuple(readings.back(), objectPath));
                }
            }
        }
    }
};

} // namespace nodemanager
