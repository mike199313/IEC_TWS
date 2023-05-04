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

#include <chrono>

namespace nodemanager
{

static constexpr const DeviceIndex kMaxPlatformNumber = 1;
static constexpr const DeviceIndex kMaxCpuNumber = 8;
static constexpr const DeviceIndex kMaxPsuNumber = 3;
static constexpr const DeviceIndex kMaxPcieNumber = 8;
static constexpr const DeviceIndex kMaxGpioNumber = kAllDevices;
static constexpr const DeviceIndex kComponentIdIgnored = 0;
static constexpr unsigned int kSmartDeviceIndex = 0;
static constexpr unsigned int kPowerStateDeviceIndex = 0;
static constexpr double kMaxEnergySensorReadingValue{
    std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        std::chrono::duration<double, std::micro>{
            std::numeric_limits<std::int32_t>::max()})
        .count()};

} // namespace nodemanager
