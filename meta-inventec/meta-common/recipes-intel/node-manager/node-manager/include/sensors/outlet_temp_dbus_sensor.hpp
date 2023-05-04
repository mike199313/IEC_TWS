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

static constexpr const auto kOutletTempSensorBusName =
    "xyz.openbmc_project.ExitAirTempSensor";
static constexpr const auto kOutletTempSensorValueIface =
    "xyz.openbmc_project.Sensor.Value";
static constexpr unsigned int kOutletTempDeviceIndex = 0;

class OutletTempDbusSensor : public DbusSensor<double>
{
  public:
    OutletTempDbusSensor() = delete;
    OutletTempDbusSensor(const OutletTempDbusSensor&) = delete;
    OutletTempDbusSensor& operator=(const OutletTempDbusSensor&) = delete;
    OutletTempDbusSensor(OutletTempDbusSensor&&) = delete;
    OutletTempDbusSensor& operator=(OutletTempDbusSensor&&) = delete;

    OutletTempDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg) :
        DbusSensor(sensorReadingsManagerArg, busArg, kOutletTempSensorBusName,
                   kOutletTempSensorValueIface)
    {
        installSensorReadings();
    }

    virtual ~OutletTempDbusSensor() = default;

    virtual void initialize() override
    {
        updateReadingsAvailability(kOutletTempDeviceIndex, false);
        dbusRegisterForSensorValueUpdateEvent();
        dbusUpdateDeviceReadings(kOutletTempDeviceIndex);
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
        readings.emplace_back(sensorReadingsManager->createSensorReading(
            SensorReadingType::outletTemperature, kOutletTempDeviceIndex));
        objectPathMapping.emplace_back(std::make_tuple(
            readings.back(),
            "/xyz/openbmc_project/sensors/temperature/Exit_Air_Temp"));

        readings.emplace_back(sensorReadingsManager->createSensorReading(
            SensorReadingType::volumetricAirflow, kOutletTempDeviceIndex));
        objectPathMapping.emplace_back(std::make_tuple(
            readings.back(),
            "/xyz/openbmc_project/sensors/airflow/System_Airflow"));
    }
}; // namespace nodemanager

} // namespace nodemanager
