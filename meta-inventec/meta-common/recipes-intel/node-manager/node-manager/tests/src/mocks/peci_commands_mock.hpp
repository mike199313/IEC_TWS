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

#include "sensors/peci/peci_commands.hpp"

#include <optional>

#include <gmock/gmock.h>

using namespace nodemanager;

class PeciCommandsMock : public PeciCommandsIf
{
  public:
    MOCK_METHOD(std::optional<uint64_t>, getC0CounterSensor,
                (const DeviceIndex cpuIndex), (const));
    MOCK_METHOD(std::optional<uint64_t>, getEpiCounterSensor,
                (const DeviceIndex cpuIndex), (const));
    MOCK_METHOD(std::optional<uint32_t>, getCpuId, (const DeviceIndex cpuIndex),
                (const));
    MOCK_METHOD(std::optional<uint32_t>, getCpuDieMask,
                (const DeviceIndex cpuIndex), (const));
    MOCK_METHOD(std::optional<uint32_t>, isTurboEnabled,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<uint32_t>, getCoreMaskLow,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<uint32_t>, getCoreMaskHigh,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<uint8_t>, getMaxNonTurboRatio,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<std::vector<uint8_t>>, getTurboRatio,
                (const DeviceIndex cpuIndex, const uint32_t cpuId,
                 uint8_t coreCount, uint8_t coreIdx),
                (const));
    MOCK_METHOD(std::optional<uint8_t>, detectMinTurboRatio,
                (const DeviceIndex cpuIndex, const uint32_t cpuId,
                 uint8_t coreCount),
                (const));
    MOCK_METHOD(std::optional<uint8_t>, detectMaxTurboRatio,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<uint8_t>, detectCores,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<uint8_t>, getTurboRatioLimit,
                (const DeviceIndex cpuIndex), (const));
    MOCK_METHOD(bool, setTurboRatio,
                (const DeviceIndex cpuIndex, const uint8_t value), (const));
    MOCK_METHOD(std::optional<uint8_t>, getMinOperatingRatio,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(std::optional<uint8_t>, getMaxEfficiencyRatio,
                (const DeviceIndex cpuIndex, const uint32_t cpuId), (const));
    MOCK_METHOD(bool, setHwpmPreference,
                (const DeviceIndex cpuIndex, uint32_t value), (const));
    MOCK_METHOD(bool, setHwpmPreferenceBias,
                (const DeviceIndex cpuIndex, uint32_t value), (const));
    MOCK_METHOD(bool, setHwpmPreferenceOverride,
                (const DeviceIndex cpuIndex, uint32_t value), (const));
    MOCK_METHOD(std::optional<uint8_t>, getProchotRatio,
                (const DeviceIndex cpuIndex), (const));
    MOCK_METHOD(bool, setProchotRatio,
                (const DeviceIndex cpuIndex, const uint8_t newRatioLimit),
                (const));
};
