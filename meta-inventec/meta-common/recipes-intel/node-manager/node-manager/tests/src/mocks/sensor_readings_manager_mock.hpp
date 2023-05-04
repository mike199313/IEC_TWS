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
#include "sensors/sensor_reading_if.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "sensors/sensor_readings_manager.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class SensorReadingsManagerMock : public SensorReadingsManagerIf
{
  public:
    MOCK_METHOD(std::shared_ptr<SensorReadingIf>, sensorReadingExists,
                (SensorReadingType type, DeviceIndex deviceIndex));
    MOCK_METHOD(std::shared_ptr<SensorReadingIf>, createSensorReading,
                (SensorReadingType type, DeviceIndex deviceIndex));
    MOCK_METHOD(void, deleteSensorReading, (SensorReadingType type));
    MOCK_METHOD(bool, forEachSensorReading,
                (SensorReadingType sensorReadingType, DeviceIndex deviceIndex,
                 std::function<void(SensorReadingIf&)>&& action));
    MOCK_METHOD(std::shared_ptr<SensorReadingIf>,
                getAvailableAndValueValidSensorReading,
                (const SensorReadingType sensorReadingType,
                 const DeviceIndex deviceIndex),
                (const));
    MOCK_METHOD(std::shared_ptr<SensorReadingIf>, getSensorReading,
                (const SensorReadingType sensorReadingType,
                 const DeviceIndex deviceIndex),
                (const));
    MOCK_METHOD(void, registerReadingConsumer,
                (std::shared_ptr<ReadingConsumer> readingConsumers,
                 ReadingType readingType, DeviceIndex deviceIndex),
                (override));
    MOCK_METHOD(void, unregisterReadingConsumer,
                (std::shared_ptr<ReadingConsumer> readingConsumers),
                (override));
    MOCK_METHOD(bool, isCpuAvailable, (const DeviceIndex idx), (const));
    MOCK_METHOD(bool, isPowerStateOn, (), (const));
    MOCK_METHOD(bool, isGpuPowerStateOn, (), (const));
};
