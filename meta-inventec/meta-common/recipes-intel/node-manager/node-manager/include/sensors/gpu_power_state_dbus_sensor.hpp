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

#include "common_types.hpp"
#include "dbus_sensor.hpp"
#include "loggers/log.hpp"
#include "sensor_reading_type.hpp"
#include "utility/devices_configuration.hpp"

namespace nodemanager
{

static constexpr const char* kGpuPowerStateObjectPath{
    "/xyz/openbmc_project/state/host0"};
static constexpr const char* kGpuPowerStateBusName{
    "xyz.openbmc_project.State.Host"};
static constexpr const char* kGpuPowerStateDbusInterface{
    "xyz.openbmc_project.State.Host"};
static constexpr const char* kGpuPowerStatePropertyName{"GpuPowerState"};
static constexpr DeviceIndex kGpuPowerDeviceIndex = 0;

class GpuPowerStateDbusSensor : public DbusSensor<bool>
{
  public:
    GpuPowerStateDbusSensor() = delete;
    GpuPowerStateDbusSensor(const GpuPowerStateDbusSensor&) = delete;
    GpuPowerStateDbusSensor& operator=(const GpuPowerStateDbusSensor&) = delete;
    GpuPowerStateDbusSensor(GpuPowerStateDbusSensor&&) = delete;
    GpuPowerStateDbusSensor& operator=(GpuPowerStateDbusSensor&&) = delete;

    GpuPowerStateDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::string& dbusServiceNameArg = kGpuPowerStateBusName) :
        DbusSensor(sensorReadingsManagerArg, busArg, dbusServiceNameArg,
                   kGpuPowerStateDbusInterface)
    {
        installSensorReadings();
    }

    virtual ~GpuPowerStateDbusSensor() = default;

    virtual void initialize() override
    {
        dbusRegisterForSensorValueUpdateEvent();
        dbusUpdateDeviceReadings(kPowerStateDeviceIndex);
    }

  protected:
    void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const bool& value) override
    {
        sensorReading->updateValue(value ? GpuPowerState::on
                                         : GpuPowerState::off);
        sensorReading->setStatus(SensorReadingStatus::valid);
    }

  private:
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matchers;

    void installSensorReadings()
    {
        std::string objectPath{kGpuPowerStateObjectPath};
        readings.emplace_back(sensorReadingsManager->createSensorReading(
            SensorReadingType::gpuPowerState, kGpuPowerDeviceIndex));
        objectPathMapping.emplace_back(
            std::make_tuple(readings.back(), objectPath));
    }
};

} // namespace nodemanager