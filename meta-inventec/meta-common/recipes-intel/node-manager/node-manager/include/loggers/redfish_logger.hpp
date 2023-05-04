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

#include "common_types.hpp"

#include <phosphor-logging/log.hpp>

namespace nodemanager
{

class RedfishLogger
{
  public:
    RedfishLogger(const RedfishLogger&) = delete;
    RedfishLogger& operator=(const RedfishLogger&) = delete;
    RedfishLogger(RedfishLogger&&) = delete;
    RedfishLogger& operator=(RedfishLogger&&) = delete;
    virtual ~RedfishLogger() = delete;

    static void logStoppingNm()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmStopping",
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     "NodeManager.0.1.NmStopping"));
    }

    static void logUnableToDisableSpsNm()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmUnableToDisableSpsNm",
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     "NodeManager.0.1.NmUnableToDisableSpsNm"));
    }

    static void logInitializationMode3()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmInitializationMode3",
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     "NodeManager.0.1.NmInitializationMode3"));
    }

    static void logSensorMissing(const std::string& sensorReadingTypeName,
                                 const DeviceIndex& deviceIndex)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmSensorMissing",
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     "NodeManager.0.1.NmSensorMissing"),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%d",
                                     sensorReadingTypeName.c_str(),
                                     deviceIndex));
    }

    static void logLimitExceptionOccurred(const std::string& policyPath)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmLimitExceptionOccurred",
            phosphor::logging::entry(
                "REDFISH_MESSAGE_ID=%s",
                "NodeManager.0.1.NmLimitExceptionOccurred"),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s",
                                     policyPath.c_str()));
    }

    static void logInitializeSoftShutdown()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmInitializeSoftShutdown",
            phosphor::logging::entry(
                "REDFISH_MESSAGE_ID=%s",
                "NodeManager.0.1.NmInitializeSoftShutdown"));
    }

    static void logPowerShutdownFailed()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmPowerShutdownFailed",
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     "NodeManager.0.1.NmPowerShutdownFailed"));
    }

    static void logPolicyAttributeAdjusted(const std::string& policyPath)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmPolicyAttributeAdjusted",
            phosphor::logging::entry(
                "REDFISH_MESSAGE_ID=%s",
                "NodeManager.0.1.NmPolicyAttributeAdjusted"),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s",
                                     policyPath.c_str()));
    }

    static void logReadingMissing(const std::string& readingTypeName)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmReadingMissing",
            phosphor::logging::entry("REDFISH_MESSAGE_ID=%s",
                                     "NodeManager.0.1.NmReadingMissing"),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s",
                                     readingTypeName.c_str()));
    }

    static void logNonCriticalReadingMissing(const std::string& readingTypeName)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmNonCriticalReadingMissing",
            phosphor::logging::entry(
                "REDFISH_MESSAGE_ID=%s",
                "NodeManager.0.1.NmNonCriticalReadingMissing"),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s",
                                     readingTypeName.c_str()));
    }

    static void logPolicyAttributeIncorrect(const std::string& policyPath,
                                            const std::string& errorDescription)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Sending RF message: NodeManager.0.1.NmPolicyAttributeIncorrect",
            phosphor::logging::entry(
                "REDFISH_MESSAGE_ID=%s",
                "NodeManager.0.1.NmPolicyAttributeIncorrect"),
            phosphor::logging::entry("REDFISH_MESSAGE_ARGS=%s,%s",
                                     policyPath.c_str(),
                                     errorDescription.c_str()));
    }
};

} // namespace nodemanager
