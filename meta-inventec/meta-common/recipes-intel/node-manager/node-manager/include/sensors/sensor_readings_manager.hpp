/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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

#include "../common_types.hpp"
#include "readings/reading_consumer.hpp"
#include "sensor_reading.hpp"
#include "sensor_reading_if.hpp"
#include "sensor_reading_type.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "utility/enum_to_string.hpp"

#include <boost/range/adaptors.hpp>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>

namespace nodemanager
{

class SensorReadingsManagerIf : public ReadingEventDispatcherIf
{
  public:
    virtual ~SensorReadingsManagerIf() = default;

    virtual std::shared_ptr<SensorReadingIf>
        createSensorReading(SensorReadingType type,
                            DeviceIndex deviceIndex) = 0;

    virtual void deleteSensorReading(SensorReadingType type) = 0;

    virtual bool forEachSensorReading(
        SensorReadingType sensorReadingType, DeviceIndex deviceIndex,
        std::function<void(SensorReadingIf&)>&& action) = 0;

    virtual std::shared_ptr<SensorReadingIf>
        getAvailableAndValueValidSensorReading(
            const SensorReadingType sensorReadingType,
            const DeviceIndex deviceIndex) const = 0;

    virtual std::shared_ptr<SensorReadingIf>
        getSensorReading(const SensorReadingType sensorReadingType,
                         const DeviceIndex deviceIndex) const = 0;

    virtual bool isCpuAvailable(const DeviceIndex idx) const = 0;
    virtual bool isPowerStateOn() const = 0;
    virtual bool isGpuPowerStateOn() const = 0;
};

class SensorReadingsManager
    : public SensorReadingsManagerIf,
      public std::enable_shared_from_this<SensorReadingsManager>
{
  public:
    SensorReadingsManager(const SensorReadingsManager&) = delete;
    SensorReadingsManager& operator=(const SensorReadingsManager&) = delete;
    SensorReadingsManager(SensorReadingsManager&&) = delete;
    SensorReadingsManager& operator=(SensorReadingsManager&&) = delete;

    SensorReadingsManager()
    {
    }

    virtual ~SensorReadingsManager() = default;

    bool isCpuAvailable(const DeviceIndex idx) const final
    {
        auto sensor = getSensorReading(SensorReadingType::cpuPackagePower, idx);
        if (!sensor || sensor->getStatus() == SensorReadingStatus::unavailable)
        {
            return false;
        }
        return true;
    }

    /**
     * @brief Returns current platform power state, the value is taken from the
     * PowerStateDbusSensor.
     *
     * @return true only when PowerStateDbusSensor sensor is set to S0.
     * @return false
     */
    bool isPowerStateOn() const final
    {
        if (auto sensor = getAvailableAndValueValidSensorReading(
                SensorReadingType::powerState, 0))
        {
            const auto v = sensor->getValue();
            if (const PowerStateType* s = std::get_if<PowerStateType>(&v))
            {
                return *s == PowerStateType::s0;
            }
            else
            {
                throw std::logic_error("SensorReadingType::powerState operates "
                                       "on different type than expected");
            }
        }
        return false;
    }

    /**
     * @brief Returns true only when gpuPowerState sensor is available and is
     * set to GpuPowerState::on value.
     *
     * @return true
     * @return false
     */
    bool isGpuPowerStateOn() const final
    {
        if (auto sensor = getAvailableAndValueValidSensorReading(
                SensorReadingType::gpuPowerState, 0))
        {
            const auto v = sensor->getValue();
            if (const GpuPowerState* s = std::get_if<GpuPowerState>(&v))
            {
                return *s == GpuPowerState::on;
            }
            else
            {
                throw std::logic_error(
                    "SensorReadingType::gpuPowerState operates "
                    "on different type than expected");
            }
        }
        return false;
    }

    virtual void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer, ReadingType type,
        DeviceIndex deviceIndex = kAllDevices) final
    {
        readingConsumers[readingConsumer] = {type, deviceIndex};
    }

    virtual void unregisterReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumer) final
    {
        readingConsumers.erase(readingConsumer);
    }

    /**
     * @brief Creates new sensor reading. Returns handle to a newly created
     * object.
     *
     * @param type
     * @param deviceIndex
     * @return std::shared_ptr<SensorReadingIf>
     */
    std::shared_ptr<SensorReadingIf>
        createSensorReading(SensorReadingType type, DeviceIndex deviceIndex)
    {
        if (eventCallback == nullptr)
        {
            eventCallback =
                [weakSelf = this->weak_from_this()](
                    SensorEventType eventType,
                    std::shared_ptr<SensorReadingIf> sensorReading) {
                    Logger::log<LogLevel::debug>(
                        "Sensor event %d from SensorReadingType: %d, "
                        "deviceIndex: %d",
                        std::underlying_type_t<SensorEventType>(eventType),
                        std::underlying_type_t<SensorReadingType>(
                            sensorReading->getSensorReadingType()),
                        unsigned{sensorReading->getDeviceIndex()});
                    SensorContext sensorCtx = {
                        sensorReading->getSensorReadingType(),
                        sensorReading->getDeviceIndex()};
                    if (auto self = weakSelf.lock())
                    {
                        for (const auto& [consumer, readingCtx] :
                             self->readingConsumers)
                        {
                            if ((readingCtx.deviceIndex == kAllDevices ||
                                 sensorCtx.deviceIndex ==
                                     readingCtx.deviceIndex) &&
                                mapReadingTypeToSensorReadingType(
                                    readingCtx.type) == sensorCtx.type)
                            {
                                consumer->reportEvent(eventType, sensorCtx,
                                                      readingCtx);
                            }
                        }
                    }
                };
        }
        if (deviceIndex == kAllDevices || getSensorReading(type, deviceIndex))
        {
            std::ostringstream msg;
            msg << "Sensor Reading with the provided type and index: "
                << enumToStr(kSensorReadingTypeNames, type) << "-"
                << std::to_string(deviceIndex)
                << " already exists or index out of range has been requested";
            throw std::runtime_error(msg.str());
        }

        return allSensorReadings[type][deviceIndex] =
                   std::make_shared<SensorReading>(type, deviceIndex,
                                                   eventCallback);
    }

    /**
     * @brief Delete sensor reading.
     *
     * @param type
     * @param deviceIndex
     */
    void deleteSensorReading(SensorReadingType type)
    {
        allSensorReadings.erase(type);
    }

    /**
     * @brief Executes provided function for each sensor reading found. For the
     * provided reading type checks only readings with that type. Returns true
     * if any sensor reading has been found.
     *
     * @param sensorReadingType
     * @param deviceIndex
     * @param action
     * @return bool
     */
    bool forEachSensorReading(SensorReadingType sensorReadingType,
                              DeviceIndex deviceIndex,
                              std::function<void(SensorReadingIf&)>&& action)
    {
        bool anySensorFound = false;

        for (auto& [index, sensorReadingHandle] :
             allSensorReadings[sensorReadingType])
        {
            SensorReadingIf& sensorReading = *sensorReadingHandle;
            if (index == deviceIndex || deviceIndex == kAllDevices)
            {
                action(sensorReading);
                anySensorFound = true;
            }
        }

        return anySensorFound;
    }

    /**
     * @brief Checks if the specified sensor reading reports being available and
     * value valid. If user specify request for all readings (kAllDevices),
     * returns false.
     *
     * @param sensorReadingType
     * @param deviceIndex
     * @return Returns pointer to a sensor reading if found and meets criteria,
     *         otheriwse nullptr.
     */
    std::shared_ptr<SensorReadingIf> getAvailableAndValueValidSensorReading(
        const SensorReadingType sensorReadingType,
        const DeviceIndex deviceIndex) const
    {
        if (auto sensorReading =
                getSensorReading(sensorReadingType, deviceIndex))
        {
            if (sensorReading->isGood())
            {
                return sensorReading;
            }
        }

        return nullptr;
    }

    /**
     * @brief Checks if the specified sensor reading reports being available and
     * value valid. If user specify request for all readings (kAllDevices),
     * returns false.
     *
     * @param sensorReadingType
     * @param deviceIndex
     * @return Returns pointer to a sensor reading if found and meets criteria,
     *         otheriwse nullptr.
     */
    std::shared_ptr<SensorReadingIf>
        getSensorReading(const SensorReadingType sensorReadingType,
                         const DeviceIndex deviceIndex) const
    {
        auto sensorReadingsIterator = allSensorReadings.find(sensorReadingType);

        if (sensorReadingsIterator != allSensorReadings.end())
        {
            auto sensorReadingsByType = sensorReadingsIterator->second;
            auto sensorReading = sensorReadingsByType.find(deviceIndex);

            if (sensorReading != sensorReadingsByType.end())
            {
                return sensorReading->second;
            }
        }
        return nullptr;
    }

  private:
    std::unordered_map<std::shared_ptr<ReadingConsumer>, ReadingContext>
        readingConsumers;
    SensorReadingEventCallback eventCallback;
    std::unordered_map<
        SensorReadingType,
        std::unordered_map<DeviceIndex, std::shared_ptr<SensorReadingIf>>>
        allSensorReadings;
};

} // namespace nodemanager