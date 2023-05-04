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

namespace nodemanager
{

class ThrottlingEvent
{
  public:
    ThrottlingEvent(const std::chrono::seconds& startTimestampArg,
                    const std::string& idArg,
                    const nlohmann::json& policyJsonArg,
                    const std::string& reasonArg) :
        startTimestamp(startTimestampArg),
        id(idArg), policyJson(policyJsonArg), reason(reasonArg), finished(false)
    {
        method = getMethodName();
    }

    std::string getId() const
    {
        return id;
    }

    bool isFinished() const
    {
        return finished;
    }

    void stop(const std::chrono::seconds& timestamp)
    {
        stopTimestamp = timestamp;
        finished = true;
    }

    bool isNmEvent() const
    {
        return !policyJson.is_null();
    }

    nlohmann::json toJson() const
    {
        nlohmann::json eventJson;
        eventJson["Start"] = startTimestamp.count();
        if (isFinished())
        {
            eventJson["Stop"] = stopTimestamp.count();
        }
        eventJson["Reason"] = reason;
        eventJson["Method"] = method;
        if (!policyJson.is_null())
        {
            eventJson["Policy"] = policyJson;
        }
        return eventJson;
    }

  private:
    std::chrono::seconds startTimestamp;
    std::chrono::seconds stopTimestamp;
    std::string id;
    nlohmann::json policyJson;
    std::string reason;
    std::string method;
    bool finished;

    std::string getMethodName() const
    {
        static const std::string methodDefault{"PowerLimit"};
        static const std::string methodOnSmaRT{"SmaRTEvent"};
        static const std::set<std::string> methodsSpecific{
            "HwpmPerfBias", "HwpmPerfPreference", "HwpmPerfPreferenceOverride",
            "Prochot", "TurboRatioLimit"};
        std::string methodName;

        if (policyJson.is_null())
        {
            methodName = methodOnSmaRT;
        }
        else
        {
            if (std::string pId{policyJson.value("Id", "")}; !pId.empty())
            {
                methodName = methodsSpecific.count(pId) ? pId : methodDefault;
            }
        }
        return methodName;
    }
};

} // namespace nodemanager