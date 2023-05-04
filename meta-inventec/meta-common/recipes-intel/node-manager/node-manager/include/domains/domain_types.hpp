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

#include "common_types.hpp"
#include "domains/capabilities/domain_capabilities.hpp"
#include "policies/policy_types.hpp"
#include "readings/reading_type.hpp"
#include "triggers/trigger_enums.hpp"

namespace nodemanager
{

struct DomainInfo
{
    std::string objectPath;
    ReadingType controlledParameter;
    std::shared_ptr<DomainCapabilitiesIf> capabilities;
    DomainId domainId;
    std::shared_ptr<std::vector<DeviceIndex>> availableComponents;
    bool requiredReadingUnavailable;
    std::shared_ptr<std::vector<TriggerType>> triggers;
    uint8_t maxComponentNumber;
    bool operator==(const DomainInfo& rhs) const
    {
        return objectPath == rhs.objectPath &&
               controlledParameter == rhs.controlledParameter &&
               capabilities == rhs.capabilities && domainId == rhs.domainId &&
               maxComponentNumber == rhs.maxComponentNumber &&
               std::equal(rhs.availableComponents->begin(),
                          rhs.availableComponents->end(),
                          availableComponents->begin()) &&
               requiredReadingUnavailable == rhs.requiredReadingUnavailable &&
               std::equal(rhs.triggers->begin(), rhs.triggers->end(),
                          triggers->begin());
    }
};

} // namespace nodemanager