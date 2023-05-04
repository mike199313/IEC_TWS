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

using namespace nodemanager;

static const std::string kDomainObjectPath =
    std::string("/xyz/openbmc_project/NodeManager/Domain/");

class DbusPaths
{
  public:
    static std::string domain(const DomainId id)
    {
        return kDomainObjectPath + enumToStr(kDomainIdNames, id);
    }
    static std::string policy(const DomainId domainId, const uint16_t policyId)
    {
        return domain(domainId) + "/Policy/" + std::to_string(policyId);
    }
};
