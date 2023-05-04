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

#include "common_types.hpp"
#include "policies/policy_if.hpp"
#include "utility/enum_to_string.hpp"

#include <phosphor-logging/log.hpp>

namespace nodemanager
{

class ThrottlingLogger
{
  public:
    ThrottlingLogger(const ThrottlingLogger&) = delete;
    ThrottlingLogger& operator=(const ThrottlingLogger&) = delete;
    ThrottlingLogger(ThrottlingLogger&&) = delete;
    ThrottlingLogger& operator=(ThrottlingLogger&&) = delete;
    virtual ~ThrottlingLogger() = delete;

    static std::function<void(void)>
        logStart(std::shared_ptr<PolicyBasicIf> policy)
    {
        auto policyJson = policy->toJson();
        policyJson["Id"] = policy->getId();
        auto logId = reinterpret_cast<uintptr_t>(policy.get());
        auto objectPath = policy->getShortObjectPath();
        auto msg = "Throttling started by policy: " + objectPath;
        phosphor::logging::log<phosphor::logging::level::INFO>(
            msg.c_str(), phosphor::logging::entry("NM_EVENT_TYPE=%s", "+"),
            phosphor::logging::entry("NM_ENTRY_ID=%d", logId),
            phosphor::logging::entry("NM_POLICY_JSON=%s",
                                     policyJson.dump().c_str()));
        return [objectPath, logId]() { logStop(objectPath, logId); };
    }

    static void logRestart()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Throttling restart",
            phosphor::logging::entry("NM_EVENT_TYPE=%s", "R"));
    }

  private:
    static void logStop(std::string objectPath, uintptr_t logId)
    {
        auto msg = "Throttling stopped by policy: " + objectPath;
        phosphor::logging::log<phosphor::logging::level::INFO>(
            msg.c_str(), phosphor::logging::entry("NM_EVENT_TYPE=%s", "-"),
            phosphor::logging::entry("NM_ENTRY_ID=%d", logId),
            phosphor::logging::entry("NM_POLICY_JSON=null"));
    }
};

} // namespace nodemanager
