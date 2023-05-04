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
#include "sensor.hpp"
#include "sensor_reading_type.hpp"
#include "sensor_readings_manager.hpp"
#include "utility/devices_configuration.hpp"

#include <future>
#include <thread>

namespace nodemanager
{

static constexpr const auto kSmartStatusFilePath =
    "/sys/devices/platform/smart/status";
static const std::unordered_map<std::string, SmartStatusType> kSmartStatusMap =
    {{"uninitialized", SmartStatusType::uninitialized},
     {"no_gpio", SmartStatusType::noGpio},
     {"idle", SmartStatusType::idle},
     {"interrupt_handling", SmartStatusType::interruptHandling}};

class SmartStatusSensor : public Sensor
{
  public:
    SmartStatusSensor() = delete;
    SmartStatusSensor(const SmartStatusSensor&) = delete;
    SmartStatusSensor& operator=(const SmartStatusSensor&) = delete;
    SmartStatusSensor(SmartStatusSensor&&) = delete;
    SmartStatusSensor& operator=(SmartStatusSensor&&) = delete;

    SmartStatusSensor(
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg) :
        Sensor(sensorReadingsManagerArg)
    {
        readings.emplace_back(sensorReadingsManager->createSensorReading(
            SensorReadingType::smartStatus, kSmartDeviceIndex));
    }

    virtual ~SmartStatusSensor() = default;

    void run() final
    {
        if (future)
        {
            const auto& sensorReading = readings.at(0);
            if (future->valid() && future->wait_for(std::chrono::seconds(0)) ==
                                       std::future_status::ready)
            {
                auto [value, status] = future->get();
                sensorReading->setStatus(status);
                if (status == SensorReadingStatus::valid)
                {
                    sensorReading->updateValue(value);
                }
            }
            else
            {
                sensorReading->setStatus(SensorReadingStatus::unavailable);
            }
        }

        if (!future || !future->valid())
        {
            future = std::async(std::launch::async, []() {
                std::ifstream file(kSmartStatusFilePath);
                if (!file.good())
                {
                    return std::make_pair(SmartStatusType::uninitialized,
                                          SensorReadingStatus::unavailable);
                }

                std::string status;
                file >> status;
                if (file.good())
                {
                    auto it = kSmartStatusMap.find(status);
                    if (it != kSmartStatusMap.end())
                    {
                        return std::make_pair(it->second,
                                              SensorReadingStatus::valid);
                    }
                }
                return std::make_pair(SmartStatusType::uninitialized,
                                      SensorReadingStatus::invalid);
            });
        }
    };

  private:
    std::optional<std::future<std::pair<SmartStatusType, SensorReadingStatus>>>
        future;
};

} // namespace nodemanager