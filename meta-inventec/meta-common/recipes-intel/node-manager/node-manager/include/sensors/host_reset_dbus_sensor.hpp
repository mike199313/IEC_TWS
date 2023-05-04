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

#include "common_types.hpp"
#include "dbus_sensor.hpp"
#include "utility/devices_configuration.hpp"
#include "utility/final_callback.hpp"

namespace nodemanager
{

static constexpr const auto kHostResetSensorBusName =
    "xyz.openbmc_project.State.Host";
static constexpr const auto kHostResetSensorValueIface =
    "xyz.openbmc_project.State.OperatingSystem.Status";
static constexpr DeviceIndex kHostResetDeviceIndex = 0;
static constexpr const int kHostResetSensorMaxNumOfRetries = 10;

class HostResetDbusSensor : public DbusSensor<std::string>
{
  public:
    HostResetDbusSensor(const HostResetDbusSensor&) = delete;
    HostResetDbusSensor& operator=(const HostResetDbusSensor&) = delete;
    HostResetDbusSensor(HostResetDbusSensor&&) = delete;
    HostResetDbusSensor& operator=(HostResetDbusSensor&&) = delete;

    HostResetDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg) :
        DbusSensor(sensorReadingsManagerArg, busArg, kHostResetSensorBusName,
                   kHostResetSensorValueIface)
    {
        installSensorReadings();
    }

    virtual ~HostResetDbusSensor() = default;

    virtual void initialize() override
    {
        updateReadingsAvailability(kHostResetDeviceIndex, false);
        dbusRegisterForSensorValueUpdateEvent();
        dbusUpdateDeviceReadings(kHostResetDeviceIndex,
                                 kHostResetSensorMaxNumOfRetries);
    };

  protected:
    void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const std::string& value) override
    {
        bool hostResetting = true;
        // The short string "Inactive" is deprecated in favor of the full enum
        // string
        // Support for the short string will be removed in the future.
        if ((value != "Inactive") &&
            (value != "xyz.openbmc_project.State.OperatingSystem.Status."
                      "OSStatus.Inactive"))
        {
            hostResetting = false;
        }
        sensorReading->updateValue(static_cast<double>(hostResetting));
        sensorReading->setStatus(SensorReadingStatus::valid);
    }

  private:
    void installSensorReadings()
    {
        readings = {sensorReadingsManager->createSensorReading(
            SensorReadingType::hostReset, kHostResetDeviceIndex)};
        objectPathMapping = {
            std::make_tuple(readings.back(), "/xyz/openbmc_project/state/os")};
    }
};

} // namespace nodemanager