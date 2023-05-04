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
#include "loggers/log.hpp"

namespace nodemanager
{

static constexpr const auto kInletTempSensorBusName =
    "xyz.openbmc_project.HwmonTempSensor";
static constexpr const auto kInletTempSensorValueIface =
    "xyz.openbmc_project.Sensor.Value";
static constexpr DeviceIndex kInletTempDeviceIndex = 0;

class InletTempDbusSensor : public DbusSensor<double>
{
  public:
    InletTempDbusSensor() = delete;
    InletTempDbusSensor(const InletTempDbusSensor&) = delete;
    InletTempDbusSensor& operator=(const InletTempDbusSensor&) = delete;
    InletTempDbusSensor(InletTempDbusSensor&&) = delete;
    InletTempDbusSensor& operator=(InletTempDbusSensor&&) = delete;

    InletTempDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg) :
        DbusSensor(sensorReadingsManagerArg, busArg, kInletTempSensorBusName,
                   kInletTempSensorValueIface)
    {
        installSensorReadings();
    }

    virtual ~InletTempDbusSensor() = default;

    virtual void initialize() override
    {
        updateReadingsAvailability(kInletTempDeviceIndex, false);
        dbusRegisterForSensorValueUpdateEvent();
        dbusUpdateDeviceReadings(kInletTempDeviceIndex);
    }

  protected:
    void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const double& value) override
    {
        if (std::isfinite(value))
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
    void installSensorReadings()
    {
        readings = {sensorReadingsManager->createSensorReading(
            SensorReadingType::inletTemperature, kInletTempDeviceIndex)};
        objectPathMapping.emplace_back(std::make_tuple(
            readings.back(),
            "/xyz/openbmc_project/sensors/temperature/Inlet_BRD_Temp"));
    }
}; // namespace nodemanager

} // namespace nodemanager
