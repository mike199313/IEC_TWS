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

#include <optional>

#include <gmock/gmock.h>

using namespace nodemanager;

class SensorReadingMock : public SensorReadingIf
{
  public:
    MOCK_METHOD(void, setStatus, (const SensorReadingStatus));
    MOCK_METHOD(SensorReadingStatus, getStatus, (), (const));
    MOCK_METHOD(NmHealth, getHealth, (), (const));
    MOCK_METHOD(bool, isGood, (), (const));
    MOCK_METHOD(ValueType, getValue, (), (const));
    MOCK_METHOD(void, updateValue, (ValueType newValue));
    MOCK_METHOD(SensorReadingType, getSensorReadingType, (), (const));
    MOCK_METHOD(DeviceIndex, getDeviceIndex, (), (const));
};