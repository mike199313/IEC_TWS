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

#include "domains/domain_ac_total_power.hpp"
#include "domains/domain_cpu_subsystem.hpp"
#include "domains/domain_dc_total_power.hpp"
#include "domains/domain_hw_protection.hpp"
#include "domains/domain_memory_subsystem.hpp"
#include "domains/domain_pcie.hpp"
#include "domains/domain_power.hpp"
#include "domains/domain_types.hpp"

namespace nodemanager
{

static std::unordered_map<std::type_index, std::optional<ReadingType>>
    domainClassToMinCapability = {
        {typeid(DomainAcTotalPower), std::nullopt},
        {typeid(DomainCpuSubsystem),
         ReadingType::cpuPackagePowerCapabilitiesMin},
        {typeid(DomainMemorySubsystem), std::nullopt},
        {typeid(DomainHwProtection), std::nullopt},
        {typeid(DomainPcie), ReadingType::pciePowerCapabilitiesMin},
        {typeid(DomainDcTotalPower), std::nullopt},
};

static std::unordered_map<std::type_index, std::optional<ReadingType>>
    domainClassToMaxCapability = {
        {typeid(DomainAcTotalPower),
         ReadingType::acPlatformPowerCapabilitiesMax},
        {typeid(DomainCpuSubsystem),
         ReadingType::cpuPackagePowerCapabilitiesMax},
        {typeid(DomainMemorySubsystem),
         ReadingType::dramPackagePowerCapabilitiesMax},
        {typeid(DomainHwProtection),
         ReadingType::hwProtectionPowerCapabilitiesMax},
        {typeid(DomainPcie), ReadingType::pciePowerCapabilitiesMax},
        {typeid(DomainDcTotalPower),
         ReadingType::dcPlatformPowerCapabilitiesMax},
};

static std::unordered_map<std::type_index, uint32_t>
    domainClassToMinCorrectionTime = {
        {typeid(DomainAcTotalPower), 6000},
        {typeid(DomainCpuSubsystem), 1000},
        {typeid(DomainMemorySubsystem), 1000},
        {typeid(DomainHwProtection), 1000},
        {typeid(DomainPcie), 1000},
        {typeid(DomainDcTotalPower), 1000},
};
} // namespace nodemanager