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
#include "devices_manager/gpio_provider.hpp"
#include "nlohmann/json.hpp"
#include "sensor_reading_type.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "utility/status_provider_if.hpp"

#include <chrono>
#include <variant>

namespace nodemanager
{

struct CpuUtilizationType
{
    uint64_t c0Delta;
    std::chrono::microseconds duration;
    uint64_t maxCpuUtilization;

    inline bool operator==(const CpuUtilizationType& rhs) const
    {
        return c0Delta == rhs.c0Delta && duration == rhs.duration &&
               maxCpuUtilization == rhs.maxCpuUtilization;
    }
};

void to_json(nlohmann::json& j, const CpuUtilizationType& data)
{
    j["c0Delta"] = data.c0Delta;
    j["duration"] = data.duration.count();
    j["maxCpuUtilization"] = data.maxCpuUtilization;
}

using ValueType =
    std::variant<double, CpuUtilizationType, PowerStateType, SmartStatusType,
                 GpuPowerState, uint8_t, uint32_t>;

class SensorReadingIf
{
  public:
    virtual ~SensorReadingIf() = default;

    /**
     * @brief Set the status about sensor reading availability and value
     * validity
     *
     * @param newStatus
     */
    virtual void setStatus(const SensorReadingStatus newStatus) = 0;

    /**
     * @brief Get the sensor reading status. Returns std::nullopt when status
     * has not been initialized yet.
     *
     * @return SensorReadingStatus
     */
    virtual SensorReadingStatus getStatus() const = 0;

    /**
     * @brief Get the sensor health
     *
     * @return NmHealth
     */
    virtual NmHealth getHealth() const = 0;

    /**
     * @brief Returns true if sensor reading status is equal to
     * SensorReadingStatus::valid.
     *
     * @return true
     * @return false
     */
    virtual bool isGood() const = 0;

    /**
     * @brief Get the Value object
     *
     * @return double
     */
    virtual ValueType getValue() const = 0;

    /**
     * @brief Update the value
     *
     * @param newValue
     */
    virtual void updateValue(ValueType newValue) = 0;

    /**
     * @brief Get the Sensor Reading Type object
     *
     * @return SensorReadingType
     */
    virtual SensorReadingType getSensorReadingType() const = 0;

    /**
     * @brief Get the Device Index object
     *
     * @return DeviceIndex
     */
    virtual DeviceIndex getDeviceIndex() const = 0;

  private:
};

} // namespace nodemanager