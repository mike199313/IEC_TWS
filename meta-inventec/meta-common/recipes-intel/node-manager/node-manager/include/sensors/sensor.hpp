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
#include "flow_control.hpp"
#include "sensor_reading_if.hpp"
#include "sensor_reading_type.hpp"
#include "sensor_readings_manager.hpp"
#include "utility/status_provider_if.hpp"

#include <boost/range/adaptors.hpp>
#include <map>
#include <memory>
#include <ranges>

namespace nodemanager
{

class Sensor : public RunnerIf, public StatusProviderIf
{
  public:
    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;
    Sensor(Sensor&&) = delete;
    Sensor& operator=(Sensor&&) = delete;

    Sensor(const std::shared_ptr<SensorReadingsManagerIf>&
               sensorReadingsManagerArg) :
        sensorReadingsManager(sensorReadingsManagerArg)
    {
    }
    virtual ~Sensor() = default;

    virtual void run() override{};
    virtual void initialize(){};

    /**
     * @brief Get the Health object. Iterates over all dbus sensorReadings and
     * returns the most restrictive health status.
     *
     * @return NmHealth
     */
    virtual NmHealth getHealth() const override
    {
        std::set<NmHealth> allHealth;
        for (const auto& sensorReading : readings)
        {
            allHealth.insert(sensorReading->getHealth());
        }
        return getMostRestrictiveHealth(allHealth);
    }

    virtual void reportStatus(nlohmann::json& out) const override
    {
        for (auto& sensorReading : readings)
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
            out["Sensors"][type].push_back(tmp);
        }
    }

  protected:
    std::vector<std::shared_ptr<SensorReadingIf>> readings;
    std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager;

    /**
     * @brief Updates status of the specified sensor readings,
     *        based on device index provided by a user.
     *
     * @param deviceIndex
     * @param isPresent
     */
    void updateReadingsAvailability(const DeviceIndex deviceIndex,
                                    bool isPresent)
    {
        std::for_each(
            readings.cbegin(), readings.cend(),
            [deviceIndex, isPresent](std::shared_ptr<SensorReadingIf> reading) {
                if (reading->getDeviceIndex() == deviceIndex)
                {
                    reading->setStatus(
                        (isPresent ? SensorReadingStatus::invalid
                                   : SensorReadingStatus::unavailable));
                }
            });
    }
};
} // namespace nodemanager