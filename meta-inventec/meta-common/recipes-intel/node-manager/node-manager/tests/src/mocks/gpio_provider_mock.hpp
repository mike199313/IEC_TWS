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

#include "devices_manager/gpio_provider.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class GpioProviderMock : public GpioProviderIf
{
  public:
    MOCK_METHOD(std::string, getLineName, (DeviceIndex), (const, override));
    MOCK_METHOD(std::optional<DeviceIndex>, getGpioLine, (std::string),
                (const, override));
    MOCK_METHOD(std::string, getFormattedLineName, (DeviceIndex),
                (const, override));
    MOCK_METHOD(DeviceIndex, getGpioLinesCount, (), (const, override));
    MOCK_METHOD(std::optional<GpioState>, getState, (DeviceIndex),
                (const, override));
    MOCK_METHOD(bool, reserveGpio, (DeviceIndex), (override));
    MOCK_METHOD(void, freeGpio, (DeviceIndex), (override));
    MOCK_METHOD(bool, isGpioReserved, (DeviceIndex), (const, override));
};