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
#include "policy_enums.hpp"
#include "triggers/trigger_enums.hpp"

#include <map>
#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

namespace nlohmann
{
template <typename T>
struct adl_serializer<std::variant<std::vector<T>, T>>
{
    static void to_json(json& j, const std::variant<std::vector<T>, T>& data)
    {
        if (std::holds_alternative<T>(data))
        {
            j = std::get<T>(data);
        }
        else
        {
            j = std::get<std::vector<T>>(data);
        }
    }

    static void from_json(const json& j, std::variant<std::vector<T>, T>& data)
    {
        if (j.is_array())
        {
            data = j.get<std::vector<T>>();
        }
        else
        {
            data = j.get<T>();
        }
    }
};
} // namespace nlohmann

namespace nodemanager
{

static constexpr uint8_t kNodeManagerMaxPolicies = 64;

using PolicySuspendPeriods = std::vector<
    std::map<std::string, std::variant<std::vector<std::string>, std::string>>>;

using PolicyThresholds = std::map<std::string, std::vector<uint16_t>>;

using PolicyParamsTuple = std::tuple<
    uint32_t,                                    // 0 - correctionInMs
    uint16_t,                                    // 1 - limit
    uint16_t,                                    // 2 - statReportingPeriod
    std::underlying_type_t<PolicyStorage>,       // 3 - policyStorage
    std::underlying_type_t<PowerCorrectionType>, // 4 - powerCorrectionType
    std::underlying_type_t<LimitException>,      // 5 - limitException
    PolicySuspendPeriods,                        // 6 - suspendPeriods
    PolicyThresholds,                            // 7 - thresholds
    uint8_t,                                     // 8 - componentId
    uint16_t,                                    // 9 - triggerLimit
    std::string                                  // 10- triggerType
    >;

struct PolicyParams
{
    uint32_t correctionInMs;
    uint16_t limit;
    uint16_t statReportingPeriod;
    PolicyStorage policyStorage;
    PowerCorrectionType powerCorrectionType;
    LimitException limitException;
    PolicySuspendPeriods suspendPeriods;
    PolicyThresholds thresholds;
    DeviceIndex componentId;
    uint16_t triggerLimit;
    std::string triggerType;

    bool operator==(const PolicyParams& rhs) const
    {
        return correctionInMs == rhs.correctionInMs && limit == rhs.limit &&
               statReportingPeriod == rhs.statReportingPeriod &&
               policyStorage == rhs.policyStorage &&
               powerCorrectionType == rhs.powerCorrectionType &&
               limitException == rhs.limitException &&
               suspendPeriods == rhs.suspendPeriods &&
               thresholds == rhs.thresholds && componentId == rhs.componentId &&
               triggerLimit == rhs.triggerLimit &&
               triggerType == rhs.triggerType;
    }
};

PolicyParams& operator<<(PolicyParams& out, const PolicyParamsTuple& in)
{
    out.correctionInMs = std::get<0>(in);
    out.limit = std::get<1>(in);
    out.statReportingPeriod = std::get<2>(in);
    out.policyStorage = static_cast<PolicyStorage>(std::get<3>(in));
    out.powerCorrectionType = static_cast<PowerCorrectionType>(std::get<4>(in));
    out.limitException = static_cast<LimitException>(std::get<5>(in));
    out.suspendPeriods = std::get<6>(in);
    out.thresholds = std::get<7>(in);
    out.componentId = static_cast<DeviceIndex>(std::get<8>(in));
    out.triggerLimit = std::get<9>(in);
    out.triggerType = std::get<10>(in);
    return out;
}

void to_json(nlohmann::json& j, const PolicyParams& data)
{
    j = nlohmann::json{{"correctionInMs", data.correctionInMs},
                       {"limit", data.limit},
                       {"statReportingPeriod", data.statReportingPeriod},
                       {"policyStorage", data.policyStorage},
                       {"powerCorrectionType", data.powerCorrectionType},
                       {"limitException", data.limitException},
                       {"suspendPeriods", data.suspendPeriods},
                       {"thresholds", data.thresholds},
                       {"componentId", data.componentId},
                       {"triggerLimit", data.triggerLimit},
                       {"triggerType", data.triggerType}};
}

} // namespace nodemanager