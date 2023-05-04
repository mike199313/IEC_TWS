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

#include <boost/algorithm/string/case_conv.hpp>
#include <regex>
#include <sdbusplus/server.hpp>

namespace nodemanager
{

static constexpr auto kPowerStateObjectPath =
    "/xyz/openbmc_project/control/host0/acpi_power_state";
static constexpr auto kPowerStateBusName = "xyz.openbmc_project.Settings";
static constexpr auto kPowerStateDbusInterface =
    "xyz.openbmc_project.Control.Power.ACPIPowerState";
static constexpr auto kPowerStatePropertyName = "SysACPIStatus";

static const std::unordered_map<std::string, PowerStateType> kPowerStates = {
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S0_G0_D0",
     PowerStateType::s0},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S1_D1",
     PowerStateType::s1},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S2_D2",
     PowerStateType::s2},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S3_D3",
     PowerStateType::s3},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S4",
     PowerStateType::s4},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S5_G2",
     PowerStateType::s5},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S4_S5",
     PowerStateType::s4},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.G3",
     PowerStateType::g3},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.SLEEP",
     PowerStateType::unknown},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.G1_SLEEP",
     PowerStateType::unknown},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.OVERRIDE",
     PowerStateType::unknown},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.LEGACY_ON",
     PowerStateType::unknown},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.LEGACY_OFF",
     PowerStateType::unknown},
    {"xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.Unknown",
     PowerStateType::unknown}};

class PowerStateDbusSensor : public DbusSensor<std::string>
{
  public:
    PowerStateDbusSensor() = delete;
    PowerStateDbusSensor(const PowerStateDbusSensor&) = delete;
    PowerStateDbusSensor& operator=(const PowerStateDbusSensor&) = delete;
    PowerStateDbusSensor(PowerStateDbusSensor&&) = delete;
    PowerStateDbusSensor& operator=(PowerStateDbusSensor&&) = delete;

    PowerStateDbusSensor(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::string& dbusServiceNameArg = kPowerStateBusName) :
        DbusSensor(sensorReadingsManagerArg, busArg, dbusServiceNameArg,
                   kPowerStateDbusInterface)
    {
        installSensorReadings();
    }

    virtual ~PowerStateDbusSensor() = default;

    virtual void initialize() override
    {
        dbusRegisterForSensorValueUpdateEvent();
        dbusUpdateDeviceReadings(kPowerStateDeviceIndex);
    }

  protected:
    void interpretSensorValue(
        const std::shared_ptr<SensorReadingIf>& sensorReading,
        const std::string& value) override
    {
        auto it = kPowerStates.find(value);
        if (it != kPowerStates.end())
        {
            sensorReading->updateValue(it->second);
            sensorReading->setStatus(SensorReadingStatus::valid);
            return;
        }
        sensorReading->setStatus(SensorReadingStatus::invalid);
    }

  private:
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matchers;

    void installSensorReadings()
    {
        std::string objectPath = kPowerStateObjectPath;
        readings.emplace_back(sensorReadingsManager->createSensorReading(
            SensorReadingType::powerState, 0));
        objectPathMapping.emplace_back(
            std::make_tuple(readings.back(), objectPath));
    }
};

} // namespace nodemanager