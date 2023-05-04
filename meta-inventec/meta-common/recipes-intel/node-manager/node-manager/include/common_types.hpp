/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2021 Intel Corporation.
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

#include <boost/serialization/strong_typedef.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>

namespace nodemanager
{

enum class DomainId : uint8_t
{
    AcTotalPower = 0,
    CpuSubsystem = 1,
    MemorySubsystem = 2,
    HwProtection = 3,
    Pcie = 4,
    DcTotalPower = 5,
    Performance = 6
};

static const std::unordered_map<DomainId, std::string> kDomainIdNames = {
    {DomainId::AcTotalPower, "ACTotalPlatformPower"},
    {DomainId::CpuSubsystem, "CPUSubsystem"},
    {DomainId::MemorySubsystem, "MemorySubsystem"},
    {DomainId::HwProtection, "HWProtection"},
    {DomainId::Pcie, "PCIe"},
    {DomainId::DcTotalPower, "DCTotalPlatformPower"},
    {DomainId::Performance, "CPUPerformance"}};

enum class RaplDomainId : uint8_t
{
    cpuSubsystem = 1,
    memorySubsystem = 2,
    pcie = 4,
    dcTotalPower = 5
};

static const std::set<DomainId> kDomainId = {
    DomainId::AcTotalPower, DomainId::CpuSubsystem, DomainId::MemorySubsystem,
    DomainId::HwProtection, DomainId::Pcie,         DomainId::DcTotalPower};

enum class BudgetingStrategy : uint8_t
{
    aggressive = 0,
    nonAggressive = 1,
    immediate = 2,
};

struct Limit
{
    double value;
    BudgetingStrategy strategy;

    inline bool operator==(const Limit& rhs) const
    {
        return value == rhs.value && strategy == rhs.strategy;
    }

    inline bool operator!=(const Limit& rhs) const
    {
        return value != rhs.value || strategy != rhs.strategy;
    }
};

enum class SensorEventType
{
    readingMissing = 0,
    readingAvailable,
    sensorAppear,
    sensorDisappear
};

enum class SensorReadingStatus
{
    unavailable,
    invalid,
    valid
};

enum class ReadingEventType
{
    readingUnavailable = 0,
    readingAvailable,
    readingSourceChanged,
};

static const std::unordered_map<SensorReadingStatus, std::string>
    sensorReadingStatusNames = {
        {SensorReadingStatus::unavailable, "Unavailable"},
        {SensorReadingStatus::invalid, "Invalid"},
        {SensorReadingStatus::valid, "Valid"}};

using DeviceIndex = uint8_t;

static constexpr DeviceIndex kAllDevices = 0xff;

static constexpr DeviceIndex kComponentIdAll = kAllDevices;

using RunCompleteCallback = std::function<void(void)>;

using PolicyId = std::string;

static constexpr char kRootObjectPath[] = "/xyz/openbmc_project/NodeManager";

static constexpr const double kMaxPowerLimitWatts = 32767;

} // namespace nodemanager
