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

#include <map>
#include <set>
// #include <utility/enum.hpp>

namespace nodemanager
{

enum class TriggerType : uint8_t
{
    always = 0,
    inletTemperature = 1,
    missingReadingsTimeout = 2,
    timeAfterHostReset = 3,
    // according to HAS doc, IDs 4 and 5 are reserved for potential future use
    gpio = 6,
    cpuUtilization = 7,
    hostReset = 8,
    smbalertInterrupt = 9
};

static const std::unordered_map<TriggerType, std::string> kTriggerTypeNames = {
    {TriggerType::always, "AlwaysOn"},
    {TriggerType::inletTemperature, "InletTemperature"},
    {TriggerType::missingReadingsTimeout, "MissingReadingsTimeout"},
    {TriggerType::timeAfterHostReset, "TimeAfterHostReset"},
    {TriggerType::gpio, "GPIO"},
    {TriggerType::cpuUtilization, "CPUUtilization"},
    {TriggerType::hostReset, "HostReset"},
    {TriggerType::smbalertInterrupt, "SMBAlertInterrupt"}};

enum class TriggerActionType
{
    trigger,
    deactivate,
    missingReading
};

} // namespace nodemanager