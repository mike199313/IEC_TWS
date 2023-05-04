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

#include <nlohmann/json.hpp>

namespace nodemanager
{

enum class NmHealth
{
    ok,
    warning
};

static const std::unordered_map<NmHealth, std::string> healthNames = {
    {NmHealth::ok, "OK"}, {NmHealth::warning, "WARNING"}};

static NmHealth getMostRestrictiveHealth(const std::set<NmHealth>& allHealth)
{
    return (allHealth.count(NmHealth::ok) == allHealth.size())
               ? NmHealth::ok
               : NmHealth::warning;
}

class StatusProviderIf
{
  public:
    virtual ~StatusProviderIf() = default;
    virtual void reportStatus(nlohmann::json& out) const = 0;
    virtual NmHealth getHealth() const = 0;
};

} // namespace nodemanager