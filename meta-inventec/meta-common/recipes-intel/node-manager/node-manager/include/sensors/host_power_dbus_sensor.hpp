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
#include "dbus_sensor.hpp"
#include "utility/devices_configuration.hpp"
#include "utility/final_callback.hpp"

namespace nodemanager
{

static constexpr const auto kHostPowerSensorBusName =
    "xyz.openbmc_project.State.Chassis";
static constexpr const auto kHostPowerSensorValueIface =
    "xyz.openbmc_project.State.Chassis";
static constexpr const auto kHostPowerOn =
    "xyz.openbmc_project.State.Chassis.PowerState.On";
static constexpr DeviceIndex kHostPowerDeviceIndex = 0;
static constexpr const int kHostPowerSensorMaxNumOfRetries = 10;

class HostPowerDbusSensor : public DbusSensor<std::string>
{
  public:
    HostPowerDbusSensor(const HostPowerDbusSensor&) = delete;
    HostPowerDbusSensor& operator=(const HostPowerDbusSensor&) = delete;
    HostPowerDbusSensor(HostPowerDbusSensor&&) = delete;
    HostPowerDbusSensor& operator=(HostPowerDbusSensor&&) = delete;

    HostPowerDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg) :
        DbusSensor(sensorReadingsManagerArg, busArg, kHostPowerSensorBusName,
                   kHostPowerSensorValueIface)
    {
        installSensorReadings();
    }

    virtual ~HostPowerDbusSensor() = default;

    virtual void initialize() override
    {
        updateReadingsAvailability(kHostPowerDeviceIndex, false);
        dbusRegisterForSensorValueUpdateEvent();
        dbusUpdateDeviceReadings(kHostPowerDeviceIndex,
                                 kHostPowerSensorMaxNumOfRetries);
    };

  protected:
    void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const std::string& value) override
    {
        sensorReading->updateValue(static_cast<double>(isHostPowerOn(value)));
        sensorReading->setStatus(SensorReadingStatus::valid);
    }

  private:
    bool isHostPowerOn(const std::string& value)
    {
        return value == kHostPowerOn;
    }

    void installSensorReadings()
    {
        readings = {sensorReadingsManager->createSensorReading(
            SensorReadingType::hostPower, kHostPowerDeviceIndex)};
        objectPathMapping = {std::make_tuple(
            readings.back(), "/xyz/openbmc_project/state/chassis0")};
    }
};

} // namespace nodemanager