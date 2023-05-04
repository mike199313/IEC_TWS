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

#include "nm_dbus_const.hpp"
#include "nm_dbus_service.hpp"
#include "nm_logger.hpp"
#include "nm_scope_logger.hpp"

#include <boost/container/flat_map.hpp>
#include <filesystem>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <nm_cc.hpp>
#include <nm_commands.hpp>
#include <nm_types.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <regex>
#include <utility.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using namespace sdbusplus::xyz::openbmc_project::Common::Error;

namespace nmipmi
{

using PolicySuspendPeriods = std::vector<
    std::map<std::string, std::variant<std::vector<std::string>, std::string>>>;

using PolicyThresholds = std::map<std::string, std::vector<uint16_t>>;

void registerNmIpmiFunctions() __attribute__((constructor));

/**
 * @brief Enable/Disable the Node Manager policy control feature
 *
 * @param policyEnableDisable - Enable/Disable NM policy control mode
 * @param reserved1
 * @param domainId
 * @param reserved2
 * @param policyId
 *
 * @returns IPMI response
 */

ipmi::RspType<> enableNmPolicyControl(ipmi::Context::ptr ctx,
                                      uint3_t policyEnableDisableArg,
                                      uint5_t reserved1, uint4_t domainId,
                                      uint4_t reserved2, uint8_t policyId)
{
    LOG_ENTRY;
    if (reserved1 != 0 || reserved2 != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    NmService nmService(ctx);
    ipmi::Cc cc = nmService.verifyDomainAndPolicyPresence(domainId, policyId);

    PolicyEnableDisable policyEnableDisable{
        static_cast<int>(policyEnableDisableArg)};
    std::string pathToObjectToSetState;
    bool enableObject;

    switch (policyEnableDisable)
    {
        case PolicyEnableDisable::globalDisable:
        case PolicyEnableDisable::globalEnable:
            pathToObjectToSetState = nmService.getRootPath();
            enableObject =
                policyEnableDisable == PolicyEnableDisable::globalEnable;
            break;
        case PolicyEnableDisable::domainDisable:
        case PolicyEnableDisable::domainEnable:
            if (cc == ccInvalidDomainId)
            {
                return ipmi::response(cc);
            }
            pathToObjectToSetState = nmService.getDomainPath(domainId);
            enableObject =
                policyEnableDisable == PolicyEnableDisable::domainEnable;
            break;
        case PolicyEnableDisable::policyDisable:
        case PolicyEnableDisable::policyEnable:
            if (cc != ipmi::ccSuccess)
            {
                return ipmi::response(cc);
            }
            pathToObjectToSetState =
                nmService.getPolicyPath(domainId, policyId);
            enableObject =
                policyEnableDisable == PolicyEnableDisable::policyEnable;
            break;
        default:
            LOGGER_ERR << "Invalid Policy Enable/Disable parameter";
            return ipmi::responseInvalidFieldRequest();
    }

    // Try to set state for NodeManager, Domain or Policy object
    boost::system::error_code ec =
        setDbusProperty(ctx, nmService.getServiceName(), pathToObjectToSetState,
                        kObjectEnableInterface, "Enabled", enableObject);
    if (ec)
    {
        LOGGER_ERR << "Failed to disable/enable Node Manager component, error: "
                   << ec.message() << ", path: " << pathToObjectToSetState;
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

/**
 * @brief Set Node Manager Policy
 *
 * @param domainId
 * @param policyEnabled
 * @param reserved1
 * @param policyId
 * @param triggerType
 * @param configurationAction
 * @param powerCorrection - Aggressive CPU Power Correction
 * @param storageOption
 * @param sendAlert - Exception action: send alert
 * @param ShutdownSystem - Exception action: shutdown system
 * @param reserved2
 * @param targetLimit
 * @param correctionTimeLimit
 * @param triggerLimit
 * @param statReportingPeriod - Statistic Reporting Period in seconds
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setNmPolicy(ipmi::Context::ptr ctx, uint4_t domainId,
                            uint1_t policyEnabled, uint3_t reserved1,
                            uint8_t policyId, uint4_t triggerType,
                            uint1_t configurationAction,
                            uint2_t powerCorrection, uint1_t storageOption,
                            uint1_t sendAlert, uint1_t shutdownSystem,
                            uint6_t reserved2, uint16_t targetLimit,
                            uint32_t correctionTimeLimit, uint16_t triggerLimit,
                            uint16_t statReportingPeriod)
{
    LOG_ENTRY;
    {
        LOGGER_INFO
            << "setNmPolicy : domainId: " << static_cast<uint32_t>(domainId)
            << ", policyEnabled: " << static_cast<uint32_t>(policyEnabled)
            << ", policyId: " << static_cast<uint32_t>(policyId)
            << ", triggerType: " << static_cast<uint32_t>(triggerType)
            << ", configurationAction: "
            << static_cast<uint32_t>(configurationAction)
            << ", powerCorrection: " << static_cast<uint32_t>(powerCorrection)
            << ", storageOption: " << static_cast<uint32_t>(storageOption)
            << ", sendAlert: " << static_cast<uint32_t>(sendAlert)
            << ", shutdownSystem: " << static_cast<uint32_t>(shutdownSystem)
            << ", targetLimit: " << static_cast<uint32_t>(targetLimit)
            << ", correctionTimeLimit: " << correctionTimeLimit
            << ", triggerLimit: " << static_cast<uint32_t>(triggerLimit)
            << ", statReportingPeriod: "
            << static_cast<uint32_t>(statReportingPeriod);
    }
    if (reserved1 != 0 || reserved2 != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::system::error_code ec;
    const int limitExc = limitException(sendAlert, shutdownSystem);
    const PolicySuspendPeriods suspendPeriods;
    const PolicyThresholds thresholds;
    const uint8_t componentId = kComponentIdAll;

    const auto policyParamsTuple = std::make_tuple(
        correctionTimeLimit,               // 0 - correctionInMs
        targetLimit,                       // 1 - limit
        statReportingPeriod,               // 2 - statReportingPeriod
        static_cast<int>(storageOption),   // 3 - policyStorage
        static_cast<int>(powerCorrection), // 4 - powerCorrectionType
        limitExc,                          // 5 - limitException
        suspendPeriods,                    // 6 - suspendPeriods
        thresholds,                        // 7 - thresholds
        componentId,                       // 8 - componentId
        triggerLimit,                      // 9- triggerLimit
        numberToTriggerName(triggerType)   // 10- triggerType
    );

    NmService nmService(ctx);
    ipmi::Cc cc = nmService.verifyDomainAndPolicyPresence(domainId, policyId);
    if (cc == ccInvalidDomainId)
    {
        return ipmi::response(cc);
    }

    if (configurationAction == 0)
    {
        // Delete action
        if (cc != ipmi::ccSuccess)
        {
            return ipmi::response(cc);
        }
        return deletePolicy(ctx, domainId, policyId);
    }

    if (cc == ccInvalidPolicyId)
    {
        // Create action
        if (!isPolicyIdInRange(ctx, policyId))
        {
            return ipmi::response(ccPoliciesCannotBeCreated);
        }
        auto policyPath =
            ctx->bus->yield_method_call<sdbusplus::message::object_path>(
                ctx->yield, ec, nmService.getServiceName(),
                nmService.getDomainPath(domainId), kPolicyManagerInterface,
                "CreateWithId", std::to_string(policyId), policyParamsTuple);
        cc = getCc(ctx->cmd, ec);
        if (cc)
        {
            return ipmi::response(cc);
        }
        LOGGER_INFO << "Policy created: " << std::string{policyPath};
    }
    else
    {
        // Update action
        ctx->bus->yield_method_call<void>(
            ctx->yield, ec, nmService.getServiceName(),
            nmService.getPolicyPath(domainId, policyId),
            kPolicyAttributesInterface, "Update", policyParamsTuple);
        cc = getCc(ctx->cmd, ec);
        if (cc)
        {
            return ipmi::response(cc);
        }
        LOGGER_INFO << "Policy updated: "
                    << nmService.getPolicyPath(domainId, policyId);
    }
    ec.clear();
    ec = setDbusProperty(ctx, nmService.getServiceName(),
                         nmService.getPolicyPath(domainId, policyId).c_str(),
                         kObjectEnableInterface, "Enabled",
                         static_cast<bool>(policyEnabled));
    if (ec)
    {
        LOGGER_ERR << "Failed to enable Policy: "
                   << nmService.getPolicyPath(domainId, policyId)
                   << ", error: " << ec.message();
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

ipmi::RspType<uint4_t,  // domainId
              uint1_t,  // policyEnabled
              uint1_t,  // perDomainControl
              uint1_t,  // globalControl
              uint1_t,  // externalPolicy
              uint4_t,  // triggerType
              uint1_t,  // policyType
              uint2_t,  // powerCorrection
              uint1_t,  // storageOption
              uint1_t,  // sendAlert
              uint1_t,  // shutdown
              uint6_t,  // reserved
              uint16_t, // targetLimit
              uint32_t, // correctionTimeLimit
              uint16_t, // triggerLimit
              uint16_t  // statisticsReportingPeriod,
              >
    responseGetPolicy(ipmi::Context::ptr ctx, const uint4_t& domainId,
                      const uint8_t& policyId)
{
    LOG_ENTRY;
    uint1_t policyEnabled = 0;
    uint1_t perDomainControl = 0;
    uint1_t globalControl = 0;
    uint1_t externalPolicy = 0;
    uint4_t triggerType = 0;
    uint1_t policyType =
        1; // According to DOC should return 1 (Power Control Policy) here
    uint2_t powerCorrection = 0;
    uint1_t storageOption = 0;
    uint1_t sendAlert = 0;
    uint1_t shutdown = 0;
    uint16_t targetLimit = 0;
    uint32_t correctionTimeLimit = 0;
    uint16_t triggerLimit = 0;
    uint16_t statReportingPeriod = 0;

    NmService nmService(ctx);

    if (auto enOpt = nmService.isNmEnabled())
    {
        globalControl = (*enOpt) ? 1 : 0;
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    if (auto enOpt = nmService.isDomainEnabled(domainId))
    {
        perDomainControl = (*enOpt) ? 1 : 0;
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    if (auto enOpt = nmService.isPolicyEnabled(domainId, policyId))
    {
        policyEnabled = (*enOpt) ? 1 : 0;
    }
    else
    {
        return ipmi::responseUnspecifiedError();
    }

    ipmi::PropertyMap propMap;
    boost::system::error_code ec;
    ec = getAllDbusProperties(ctx, nmService.getServiceName(),
                              nmService.getPolicyPath(domainId, policyId),
                              kPolicyAttributesInterface, propMap);
    if (ec)
    {
        LOGGER_ERR << "Failed to getAll Policy properties, err: "
                   << ec.message();
        return ipmi::responseUnspecifiedError();
    }
    try
    {
        powerCorrection =
            tryCast<uint2_t>(std::get<int>(propMap.at("PowerCorrectionType")));
        storageOption =
            tryCast<uint1_t>(std::get<int>(propMap.at("PolicyStorage")));
        parseLimitException(std::get<int>(propMap.at("LimitException")),
                            sendAlert, shutdown);
        targetLimit = std::get<uint16_t>(propMap.at("Limit"));
        correctionTimeLimit = std::get<uint32_t>(propMap.at("CorrectionInMs"));
        statReportingPeriod =
            std::get<uint16_t>(propMap.at("StatisticsReportingPeriod"));
        triggerType =
            stringToNumber(kTriggerTypeNames,
                           std::get<std::string>(propMap.at("TriggerType")));
        if (triggerType == tryCast<uint4_t>(uint8_t{0}))
        {
            triggerLimit = targetLimit;
        }
        else
        {
            triggerLimit = std::get<uint16_t>(propMap.at("TriggerLimit"));
        }
    }
    catch (const std::exception& e)
    {
        LOGGER_ERR << "Error while parsing policy attributes, ex: " << e.what();
        return ipmi::responseUnspecifiedError();
    }
    return ipmi::responseSuccess(
        domainId, policyEnabled, perDomainControl, globalControl,
        externalPolicy, triggerType, policyType, powerCorrection, storageOption,
        sendAlert, shutdown, uint6_t{0}, targetLimit, correctionTimeLimit,
        triggerLimit, statReportingPeriod);
}

/**
 * @brief Get Node Manager Policy - Gets the NM policy parametres.
 *
 * @param domainId
 * @param reserved
 * @param policyId
 *
 * @returns IPMI response
 **/

ipmi::RspType<std::variant<std::tuple<uint4_t,  // domainId
                                      uint1_t,  // policyEnabled
                                      uint1_t,  // perDomainControl
                                      uint1_t,  // globalControl
                                      uint1_t,  // externalPolicy
                                      uint4_t,  // triggerType
                                      uint1_t,  // policyType
                                      uint2_t,  // powerCorrection
                                      uint1_t,  // storageOption
                                      uint1_t,  // sendAlert
                                      uint1_t,  // shutdown
                                      uint6_t,  // reserved
                                      uint16_t, // targetLimit
                                      uint32_t, // correctionTimeLimit
                                      uint16_t, // triggerLimit
                                      uint16_t  // statisticsReportingPeriod,
                                      >,
                           std::tuple<uint8_t, // Next Valid Policy ID
                                      uint8_t  // Number of Policies
                                      >,
                           std::tuple<uint4_t, // Next Valid domain ID
                                      uint4_t, // reserved
                                      uint8_t  // Number of domains
                                      >>>
    getNodeManagerPolicy(ipmi::Context::ptr ctx, uint4_t domainId,
                         uint4_t reserved, uint8_t policyId)
{
    LOG_ENTRY;
    if (reserved != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    boost::system::error_code ec;
    ipmi::ObjectValueTree objMap;

    NmService nmService(ctx);
    const ipmi::Cc cc =
        nmService.verifyDomainAndPolicyPresence(domainId, policyId);
    if (cc == ccInvalidDomainId)
    {
        return responseNextDomainId(nmService.getDomainPolicyMap(), domainId);
    }
    if (cc == ccInvalidPolicyId)
    {
        return responseNextPolicyId(
            nmService.getDomainPolicyMap().find(domainId)->second, policyId);
    }

    return responseGetPolicy(ctx, domainId, policyId);
}

/**
 * @brief Set Node Manager Policy Alert Thresholds
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setNmPolicyAlertThresholds()
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Node Manager Policy Alert Thresholds
 *
 * @returns IPMI response
 **/

ipmi::RspType<> getNmPolicyAlertThresholds()
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Set Node Manager Policy Suspend Periods
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setNmPolicySuspendPeriods()
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Node Manager Policy Suspend Periods
 *
 * @returns IPMI response
 **/

ipmi::RspType<> getNmPolicySuspendPeriods()
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Reset Node Manager Statistics - clears Node Manager statistics of
 *        given type
 * @param mode
 * @param reserved
 * @param domainId
 * @param reserved
 * @param policyId
 *
 * @returns IPMI response
 **/
ipmi::RspType<> resetNmStatistics(ipmi::Context::ptr ctx, uint5_t mode,
                                  uint3_t reserved1, uint4_t domainId,
                                  uint4_t reserved2, uint8_t policyId)
{
    LOG_ENTRY;
    if (reserved1 != 0 || reserved2 != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    constexpr uint5_t kResetStatModeGlobal = 0x00;
    constexpr uint5_t kResetStatModePolicy = 0x01;

    if (mode != kResetStatModeGlobal && mode != kResetStatModePolicy)
    {
        return ipmi::response(ccInvalidMode);
    }
    boost::system::error_code ec;
    NmService nmService(ctx);
    ipmi::Cc cc = nmService.verifyDomainAndPolicyPresence(domainId, policyId);
    if (mode == kResetStatModeGlobal)
    {
        if (cc == ccInvalidDomainId)
        {
            return ipmi::response(cc);
        }

        if (domainId == kGlobalStatsDomainId)
        {
            // reset global node manager statistics.
            ctx->bus->yield_method_call<void>(
                ctx->yield, ec, nmService.getServiceName(),
                nmService.getRootPath(), kStatisticsInterface,
                "ResetStatistics");
        }

        // get requested domain and reset its statistics.
        ctx->bus->yield_method_call<void>(
            ctx->yield, ec, nmService.getServiceName(),
            nmService.getDomainPath(domainId), kStatisticsInterface,
            "ResetStatistics");
    }
    else if (mode == kResetStatModePolicy)
    {
        // get requested policy and reset its statistics.
        if (cc != ipmi::ccSuccess)
        {
            return ipmi::response(cc);
        }
        ctx->bus->yield_method_call<void>(
            ctx->yield, ec, nmService.getServiceName(),
            nmService.getPolicyPath(domainId, policyId), kStatisticsInterface,
            "ResetStatistics");
    }
    cc = getCc(ctx->cmd, ec);
    if (cc)
    {
        return ipmi::response(cc);
    }

    return ipmi::responseSuccess();
}

/**
 * @brief Get Node Manager Statistics
 *
 * @param mode
 * @param reserved1
 * @param domainId
 * @param reserved2
 * @param perComponentControl
 * @param objectId - Policy/Component ID
 *
 * @returns IPMI response
 **/
ipmi::RspType<std::variant<std::tuple<uint16_t, // Current Value
                                      uint16_t, // Minimum Value
                                      uint16_t, // Maximum Value
                                      uint16_t, // Average Value
                                      uint32_t, // Timestamp
                                      uint32_t, // Statistic Reporting Period
                                      uint4_t,  // Domain ID
                                      uint1_t,  // AdministrativeState
                                      uint1_t,  // OperationalState
                                      uint1_t,  // MeasurementState
                                      uint1_t>, // ActivationState
                           std::tuple<uint64_t, // Current Value
                                      uint32_t, // Timestamp
                                      uint32_t, // Statistic Reporting Period
                                      uint4_t,  // Domain ID
                                      uint1_t,  // AdministrativeState
                                      uint1_t,  // OperationalState
                                      uint1_t,  // MeasurementState
                                      uint1_t>  // ActivationState
                           >>
    getNmStatistics(ipmi::Context::ptr ctx, uint5_t mode, uint3_t reserved1,
                    uint4_t domainId, uint3_t reserved2,
                    uint1_t perComponentControl, uint8_t objectId)
{
    LOG_ENTRY;
    if (reserved1 != 0 || reserved2 != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    const auto& modePair = statModeToDBusMap.find(mode);
    if (modePair == statModeToDBusMap.cend())
    {
        return ipmi::response(ccInvalidMode);
    }

    boost::system::error_code ec;
    NmService nmService(ctx);
    ipmi::Cc cc = nmService.verifyDomainAndPolicyPresence(domainId, objectId);
    if (mode == types::enum_cast<uint5_t>(StatsMode::perPolicyPower) ||
        mode == types::enum_cast<uint5_t>(StatsMode::perPolicyTrigger) ||
        mode == types::enum_cast<uint5_t>(StatsMode::perPolicyThrottling))
    {
        // Get from stats from policy
        if (cc != ipmi::ccSuccess)
        {
            return ipmi::response(cc);
        }
        return responsePolicyStat(ctx, statModeToDBusMap.at(mode), domainId,
                                  objectId);
    }
    if (mode == types::enum_cast<uint5_t>(StatsMode::power) ||
        mode == types::enum_cast<uint5_t>(StatsMode::throttling))
    {
        // get stats from domain
        if (cc == ccInvalidDomainId)
        {
            return ipmi::response(cc);
        }

        std::string statType = statModeToDBusMap.at(mode);
        if (perComponentControl == 1)
        {
            statType = statType + "_" + std::to_string(objectId);
        }
        return responseDomainStat(ctx, statType, domainId);
    }
    if (mode == types::enum_cast<uint5_t>(StatsMode::energyAccumulator))
    {
        if (cc == ccInvalidDomainId)
        {
            return ipmi::response(cc);
        }

        std::string statType = statModeToDBusMap.at(mode);
        if (perComponentControl == 1)
        {
            statType = statType + "_" + std::to_string(objectId);
        }
        return responseDomainEnergyStat(ctx, statType, domainId);
    }
    else
    {
        // get stats from Node Manager (global stats)
        if (cc == ccInvalidDomainId)
        {
            return ipmi::response(cc);
        }
        if (domainId != kGlobalStatsDomainId)
        {
            return ipmi::response(ccInvalidDomainId);
        }

        return responseGlobalStat(ctx, statModeToDBusMap.at(mode));
    }
}

/**
 * @brief Get Node Manager Capabilities
 *
 * @param domainId
 * @param reserved1
 * @param triggerType
 * @param policyType
 * @param reserved2
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t,  // Max Concurrent Settings
              uint16_t, // Max Limit
              uint16_t, // Min Limit
              uint32_t, // Min Correction Time
              uint32_t, // Max Correction Time
              uint16_t, // Min Statistics Reporting Period
              uint16_t, // Max Statistics Reporting Period
              uint4_t,  // Domain ID
              uint4_t>  // Reserved
    getNmCapabilities(ipmi::Context::ptr ctx, uint4_t domainId,
                      uint4_t reserved1, uint4_t triggerType,
                      uint3_t policyType, uint1_t reserved2)
{
    LOG_ENTRY;
    if (reserved1 != 0 || reserved2 != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    uint8_t maxConcurrentSettings = 0;
    uint16_t maxLimit = 0;
    uint16_t minLimit = 0;
    uint32_t minCorrectionTime = 0;
    uint32_t maxCorrectionTime = 0;
    uint16_t minStatReportingPeriod = 0;
    uint16_t maxStatReportingPeriod = 0;

    std::vector<std::string> availableTriggers;

    NmService nmService(ctx);
    boost::system::error_code ec;

    // validate parameters
    if (policyType != kPolicyTypePowerControl)
    {
        return ipmi::response(ccUnknownPolicyType);
    }

    if ((domainId == kCpuPerformanceDomainId) ||
        (domainId == kHwProtectionDomainId))
    {
        return ipmi::response(ccInvalidDomainId);
    }

    // Verify if trigger is supported by domain
    ec = ipmi::getDbusProperty<std::vector<std::string>>(
        ctx, nmService.getServiceName(), nmService.getDomainPath(domainId),
        kDomainAttributesInterface, "AvailableTriggers", availableTriggers);
    if (ec)
    {
        LOGGER_ERR << "Failed to get the list of available trigger "
                      "property, err: "
                   << ec.message();
        return ipmi::response(ccInvalidDomainId);
    }

    std::string triggerName = numberToString(kTriggerTypeNames, triggerType);
    if (triggerName.empty() ||
        std::find(availableTriggers.begin(), availableTriggers.end(),
                  triggerName) == availableTriggers.end())
    {
        return ipmi::response(ccUnsupportedPolicyTriggerType);
    }

    ec = ipmi::getDbusProperty<std::uint8_t>(
        ctx, nmService.getServiceName(), nmService.getRootPath(),
        kNodeManagerInterface, "MaxNumberOfPolicies", maxConcurrentSettings);

    if (ec)
    {
        LOGGER_ERR << "Failed to get the NodeManager Max Number of Policies "
                      "property, err: "
                   << ec.message();
        return ipmi::responseUnspecifiedError();
    }

    ipmi::PropertyMap capPropMap;
    ec = ipmi::getAllDbusProperties(ctx, nmService.getServiceName(),
                                    nmService.getDomainPath(domainId),
                                    kNmCapabilitiesInterface, capPropMap);
    if (ec)
    {
        LOGGER_ERR << "Failed to getAll Capabilities properties, err: "
                   << ec.message();
        return ipmi::response(ccInvalidDomainId);
    }

    ipmi::PropertyMap triggerPropMap;
    const static std::set<uint4_t> dummyTriggers = {0, 6, 8, 9};
    if (dummyTriggers.find(triggerType) == dummyTriggers.cend())
    {
        ec = ipmi::getAllDbusProperties(ctx, nmService.getServiceName(),
                                        nmService.getTriggerPath(triggerName),
                                        kNmTriggerInterface, triggerPropMap);

        if (ec)
        {
            LOGGER_ERR << "Failed to getAll Trigger properties, err: "
                       << ec.message();
            return ipmi::responseUnspecifiedError();
        }
    }

    try
    {
        if (dummyTriggers.find(triggerType) == dummyTriggers.cend())
        {
            maxLimit = std::get<uint16_t>(triggerPropMap.at("Max"));
            minLimit = std::get<uint16_t>(triggerPropMap.at("Min"));
        }
        else
        {
            try
            {
                maxLimit = tryCast<uint16_t>(
                    std::floor(std::get<double>(capPropMap.at("Max"))));
                minLimit = tryCast<uint16_t>(
                    std::ceil(std::get<double>(capPropMap.at("Min"))));
                if (maxLimit <= minLimit &&
                    !((maxLimit == 0) && (minLimit == 0)))
                {
                    throw std::range_error("Min is bigger or equal to Max");
                }
            }
            catch (const std::range_error& e)
            {
                LOGGER_ERR << "Failed to get Capabilities property, error: "
                           << e.what();
                maxLimit = std::numeric_limits<uint16_t>::max();
                minLimit = std::numeric_limits<uint16_t>::max();
            }
        }

        minCorrectionTime =
            std::get<uint32_t>(capPropMap.at("MinCorrectionTimeInMs"));
        maxCorrectionTime =
            std::get<uint32_t>(capPropMap.at("MaxCorrectionTimeInMs"));
        minStatReportingPeriod =
            std::get<uint16_t>(capPropMap.at("MinStatisticsReportingPeriod"));
        maxStatReportingPeriod =
            std::get<uint16_t>(capPropMap.at("MaxStatisticsReportingPeriod"));
    }
    catch (const std::exception& e)
    {
        LOGGER_ERR << "Failed to get Capabilities property, ex: " << e.what();
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(maxConcurrentSettings, maxLimit, minLimit,
                                 minCorrectionTime, maxCorrectionTime,
                                 minStatReportingPeriod, maxStatReportingPeriod,
                                 domainId, uint4_t{0});
}

/**
 * @brief Get Node Manager Version
 *
 * @returns IPMI response
 **/
ipmi::RspType<uint8_t, // Node Manager Version
              uint8_t, // IPMI Interface Version
              uint8_t, // Patch Version
              uint8_t, // Major Firmware Revision
              uint8_t> // Minor Firmware Revision
    getNmVersion(ipmi::Context::ptr ctx)
{
    LOG_ENTRY;
    uint8_t nmVersion = 0;
    uint8_t ipmiInterfaceVersion = 0x05;
    uint8_t patchVersion = 0;
    uint8_t majorFWRev = 0;
    uint8_t minorFWRev = 0;

    std::string nmVersionStr = "";
    boost::system::error_code ec;
    ipmi::DbusObjectInfo nmObject;

    NmService nmService(ctx);

    ec = ipmi::getDbusProperty<std::string>(
        ctx, nmService.getServiceName(), nmService.getRootPath(),
        kNodeManagerInterface, "Version", nmVersionStr);

    if (ec)
    {
        LOGGER_ERR << "Failed to get the NodeManager Version property, err: "
                   << ec.message();
        return ipmi::responseUnspecifiedError();
    }

    if (nmVersionStr.compare("1.0") == 0)
    {
        nmVersion = 0x10;
    }
    else
    {
        LOGGER_ERR << "Unrecognized NodeManager Version: " << nmVersionStr;
        return ipmi::responseUnspecifiedError();
    }

    std::string versionString;
    if (!intel::getActiveSoftwareVersionInfo(ctx, intel::versionPurposeBMC,
                                             versionString))
    {
        LOGGER_DEBUG << "Got active software version string: " << versionString;
        if (auto rev = intel::convertIntelVersion(versionString))
        {
            majorFWRev = rev->major;
            rev->minor = (rev->minor > 99 ? 99 : rev->minor);
            minorFWRev =
                static_cast<uint8_t>(rev->minor % 10 + (rev->minor / 10) * 16);
        }
        else
        {
            LOGGER_ERR << "Cannot parse version string, version: "
                       << versionString;
            return ipmi::responseUnspecifiedError();
        }
    }
    else
    {
        LOGGER_ERR << "Cannot get active software version";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(nmVersion, ipmiInterfaceVersion, patchVersion,
                                 majorFWRev, minorFWRev);
}

/**
 * @brief Set Node Manager Power Draw Range
 *
 * @param domainId
 * @param reserved
 * @param minPowerDraw
 * @param maxPowerDraw
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setNmPowerDrawRange(ipmi::Context::ptr ctx, uint4_t domainId,
                                    uint4_t reserved, uint16_t minPowerDraw,
                                    uint16_t maxPowerDraw)
{
    LOG_ENTRY;
    if (reserved != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    if (minPowerDraw != 0 && maxPowerDraw != 0 && minPowerDraw > maxPowerDraw)
    {
        return ipmi::response(ccParameterOutOfRange);
    }

    NmService nmService(ctx);
    std::string domainPath = nmService.getDomainPath(domainId);
    ipmi::Cc cc = nmService.verifyDomainAndPolicyPresence(domainId, uint8_t{0});
    if (cc == ccInvalidDomainId)
    {
        return ipmi::response(cc);
    }
    boost::system::error_code ec = setDbusProperty(
        ctx, nmService.getServiceName(), domainPath, kNmCapabilitiesInterface,
        "Max", static_cast<double>(maxPowerDraw));
    if (ec)
    {
        LOGGER_ERR << "Failed to set Max Power Draw, error: " << ec.message()
                   << ", objectPath: " << domainPath.c_str();
        return ipmi::responseUnspecifiedError();
    }

    ec = setDbusProperty(ctx, nmService.getServiceName(), domainPath,
                         kNmCapabilitiesInterface, "Min",
                         static_cast<double>(minPowerDraw));
    if (ec)
    {
        LOGGER_ERR << "Failed to set Min Power Draw, error: " << ec.message()
                   << ", objectPath: " << domainPath.c_str();
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

/**
 * @brief Set Turbo Synchronization Ratio
 *
 * @param cpuSocketNumber
 * @param turboRatioLimit
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setTurboSynchronizationRatio(uint8_t cpuSocketNumber,
                                             uint8_t turboRatioLimit)
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Turbo Synchronization Ratio
 *
 * @param cpuSocketNumber
 * @param activeCoresConf
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t, // Current Turbo Ratio Limit
              uint8_t, // Default Turbo Ratio Limit
              uint8_t, // Maximum Turbo Ratio Limit
              uint8_t> // Minimum Turbo Ratio Limit
    getTurboSynchronizationRatio(uint8_t cpuSocketNumber,
                                 uint8_t activeCoresConf)
{
    // TODO - Prepare proper handler implementation
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint8_t currentLimit = 0;
    // uint8_t defaultLimit = 0;
    // uint8_t maxLimit = 0;
    // uint8_t minLimit = 0;
    //
    // return ipmi::responseSuccess(currentLimit, defaultLimit, maxLimit,
    //                              minLimit);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Set Total Power Budget
 *
 * @param domainIdArg
 * @param reserved
 * @param perComponentControl
 * @param targetPowerBudget
 * @param componentIdArg
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setTotalPowerBudget(ipmi::Context::ptr ctx, uint4_t domainIdArg,
                                    uint3_t reserved,
                                    uint1_t perComponentControl,
                                    uint16_t targetPowerBudget,
                                    uint8_t componentIdArg)
{
    LOG_ENTRY;
    if (reserved != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    ipmi::Cc cc;
    const uint8_t domainId = static_cast<uint8_t>(domainIdArg);
    NmService nmService(ctx);

    boost::system::error_code ec;
    const uint8_t componentId =
        (perComponentControl == 0 || isDomainCompound(domainId))
            ? kComponentIdAll
            : componentIdArg;

    const int32_t storageOption =
        (perComponentControl == 1) ? 1 : 0; // volatile = 1,
                                            // persistent = 0
    const PolicySuspendPeriods suspendPeriods;
    const PolicyThresholds thresholds;

    const uint32_t correctionInMs = ((domainId == 0) ? 6000 : 1000);
    const auto policyParamsTuple =
        std::make_tuple(correctionInMs,           // 0 - correctionInMs
                        targetPowerBudget,        // 1 - limit
                        static_cast<uint16_t>(1), // 2 - statReportingPeriod: 1s
                        storageOption,            // 3 - policyStorage
                        2,              // 4 - powerCorrectionType: aggressive
                        0,              // 5 - limitException: no action
                        suspendPeriods, // 6 - suspendPeriods
                        thresholds,     // 7 - thresholds
                        componentId,    // 8 - componentId
                        static_cast<uint16_t>(0), // 9 - triggerLimit
                        "AlwaysOn"                // 10- triggerType: none
        );

    if (std::optional<std::vector<std::string>> policies =
            nmService.getAllPoliciesPaths())
    {
        std::string policyId = "Budget_" + std::to_string(domainId) + "_" +
                               std::to_string(componentId);
        std::string policyObjPath = nmService.getPolicyPath(domainId, policyId);
        auto it =
            std::find(policies->cbegin(), policies->cend(), policyObjPath);
        if (it == policies->cend())
        {
            cc = nmService.verifyDomainAndPolicyPresence(domainId, uint8_t{0});
            if (cc == ccInvalidDomainId)
            {
                return ipmi::response(cc);
            }
            // If there is no policy and null targetPowerBudget - return success
            if (targetPowerBudget == 0)
            {
                return ipmi::response(ipmi::ccSuccess);
            }
            // If there is no policy and not null targetPowerBudget - create
            // policy
            auto createdPolicyPath =
                ctx->bus->yield_method_call<sdbusplus::message::object_path>(
                    ctx->yield, ec, nmService.getServiceName(),
                    nmService.getDomainPath(domainId), kPolicyManagerInterface,
                    "CreateForTotalBudget", policyId, policyParamsTuple);
            cc = getCc(ctx->cmd, ec);
            return ipmi::response(cc);
        }
        else
        {
            // If there is a policy
            // If targetPowerBudget is null - remove policy
            if (targetPowerBudget == 0)
            {
                ctx->bus->yield_method_call<void>(
                    ctx->yield, ec, nmService.getServiceName(), *it,
                    kObjectDeleteInterface, "Delete");
                cc = getCc(ctx->cmd, ec);
            }
            // If targetPowerBudget not null - update policy
            else
            {
                ctx->bus->yield_method_call<void>(
                    ctx->yield, ec, nmService.getServiceName(), *it,
                    kPolicyAttributesInterface, "Update", policyParamsTuple);
                cc = getCc(ctx->cmd, ec);
            }
            return ipmi::response(cc);
        }
    }
    else
    {
        // If unexpected error happened - return cc
        return ipmi::response(ipmi::ccUnspecifiedError);
    }
}

/**
 * @brief Get Total Power Budget
 *
 * @param domainIdArg
 * @param reserved
 * @param perComponentControl
 * @param componentIdArg
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint16_t> // Target Power Budget
    getTotalPowerBudget(ipmi::Context::ptr ctx, uint4_t domainIdArg,
                        uint3_t reserved, uint1_t perComponentControl,
                        uint8_t componentIdArg)
{
    LOG_ENTRY;
    if (reserved != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }
    const uint8_t domainId = static_cast<uint8_t>(domainIdArg);

    uint8_t componentId = kComponentIdAll;
    if (perComponentControl == 1 && !isDomainCompound(domainId))
    {
        componentId = componentIdArg;
    }

    NmService nmService(ctx);
    if (std::optional<std::vector<std::string>> policies =
            nmService.getAllPoliciesPaths())
    {
        std::string policyId = "Budget_" + std::to_string(domainId) + "_" +
                               std::to_string(componentId);
        std::string policyObjPath = nmService.getPolicyPath(domainId, policyId);
        auto it =
            std::find(policies->cbegin(), policies->cend(), policyObjPath);
        if (it != policies->cend())
        {
            // policy exists, get policy limit
            uint16_t policyLimit = 0;
            boost::system::error_code ec;
            ec = ipmi::getDbusProperty<uint16_t>(
                ctx, nmService.getServiceName(), *it,
                kPolicyAttributesInterface, "Limit", policyLimit);
            if (ec)
            {
                LOGGER_ERR << "Failed to get Policy " << *it
                           << " limit attribute, error:" << ec.message();
                return ipmi::responseUnspecifiedError();
            }
            return ipmi::responseSuccess(policyLimit);
        }
        else
        {
            // verify if domain exists
            auto cc =
                nmService.verifyDomainAndPolicyPresence(domainId, uint8_t{0});
            if (cc == ccInvalidDomainId)
            {
                return ipmi::response(cc);
            }

            if (componentId != kComponentIdAll)
            {
                // verify if components are available
                std::optional<std::vector<uint8_t>> availableComponents =
                    getDomainAvailableComponents(ctx, domainIdArg);
                if (!availableComponents)
                {
                    return ipmi::response(ipmi::ccUnspecifiedError);
                }
                if (std::find(availableComponents->begin(),
                              availableComponents->end(),
                              componentId) == availableComponents->end())
                {
                    return ipmi::response(ccInvalidComponentIdentifier);
                }
            }
            // If there is no policy with provied component id just return with
            // total budget = 0
            return ipmi::responseSuccess(0);
        }
    }
    else
    {
        return ipmi::response(ipmi::ccUnspecifiedError);
    }
}

/**
 * @brief Set Max Allowed CPU P-state/Threads
 *
 * @param domainId
 * @param controlKnob
 * @param reserved
 * @param limit
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setMaxAllowedCpuPstateThreads(uint4_t domainId,
                                              uint2_t controlKnob,
                                              uint2_t /* reserved */,
                                              uint16_t limit)
{
    // TODO - Prepare proper handler implementation
    //
    // Performance Domain should be not supported!
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Max Allowed CPU P-state/Threads
 *
 * @param domainId
 * @param controlKnob
 * @param reserved
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint16_t> // Limit
    getMaxAllowedCpuPstateThreads(uint4_t domainId, uint2_t controlKnob,
                                  uint2_t /* reserved */)
{
    // TODO - Prepare proper handler implementation
    //
    // Performance Domain should be not supported!
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint16_t limit = 0;
    //
    // return ipmi::responseSuccess(limit);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Number Of CPU P-states/Threads
 *
 * @param domainId
 * @param controlKnob
 * @param reserved
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint16_t> // Value
    getNumberOfCpuPstateThreads(uint4_t domainId, uint2_t controlKnob,
                                uint2_t /* reserved */)
{
    // TODO - Prepare proper handler implementation
    //
    // Performance Domain should be not supported!
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint16_t value = 0;
    //
    // return ipmi::responseSuccess(value);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Set HW Protection Coefficient
 *
 * @param kCoefficient
 *
 * @returns IPMI response
 **/

ipmi::RspType<> setHwProtectionCoefficient(uint8_t kCoefficient)
{
    // TODO - Prepare proper handler implementation
    //
    // Below is response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess();
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get HW Protection Coefficient
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t> // K Coefficient
    getHwProtectionCoefficient(void)
{
    // TODO - Prepare proper handler implementation
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint8_t kCoefficient = 0;
    //
    // return ipmi::responseSuccess(kCoefficient);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Limiting Policy ID
 *
 * @param domainId
 * @param reserved
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t> // Policy ID
    getLimitingPolicyId(ipmi::Context::ptr ctx, uint4_t domainId,
                        uint4_t reserved)
{
    LOG_ENTRY;
    if (reserved != 0)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    boost::system::error_code ec;
    NmService nmService(ctx);
    ipmi::Cc cc = nmService.verifyDomainAndPolicyPresence(domainId, uint8_t{0});
    std::string objectPath = nmService.getDomainPath(domainId);

    if (cc == ccInvalidDomainId)
    {
        return ipmi::response(cc);
    }

    auto activePolicies =
        ctx->bus
            ->yield_method_call<std::deque<sdbusplus::message::object_path>>(
                ctx->yield, ec, nmService.getServiceName(), objectPath,
                kPolicyManagerInterface, "GetSelectedPolicies");

    if (ec)
    {
        LOGGER_ERR << "Failed to get Selected Policies, error: " << ec.message()
                   << ", objectPath: " << objectPath.c_str();
        return ipmi::responseUnspecifiedError();
    }

    cc = getCc(ctx->cmd, ec);

    if (cc)
    {
        return ipmi::response(cc);
    }

    if (!activePolicies.empty())
    {
        auto path = std::filesystem::path(activePolicies[0].str);
        if (path.has_filename() && onlyDigits(path.filename()))
        {
            try
            {
                uint8_t policyId =
                    tryCast<uint8_t>(std::stoul(path.filename()));
                return ipmi::responseSuccess(policyId);
            }
            catch (const std::exception& e)
            {
                LOGGER_ERR << "Error while parsing policy id, ex: " << e.what();
            }
        }
    }
    return ipmi::response(ccNoPolicyIsCurrentlyLimiting);
}

/**
 * @brief Get DCMI Capability Info
 *
 * @param groupExten - Group Extension Identification
 * @param paramSelector
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t, // Group Extension Identification
              uint8_t, // DCMI Specification Major Version
              uint8_t, // DCMI Specification Minor Version
              uint8_t, // Parameter Revision
              uint8_t, // Supported Rolling Average Time Periods
              uint6_t, // Supported Rolling Average Time Period Duration
              uint2_t> // Supported Rolling Average Time Period Duration
                       // Units
    getDcmiCapabilityInfo(uint8_t groupExten, uint8_t paramSelector)
{
    // TODO - Prepare proper handler implementation
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint8_t DcmiSpecMajorVersion = 0;
    // uint8_t DcmiSpecMinorVersion = 0;
    // uint8_t paramRevision = 0;
    // uint8_t averageTimePeriods = 0;
    // uint6_t averageTimePeriodDuration = 0;
    // uint2_t averageTimePeriodDurationUnits = 0;
    //
    // return ipmi::responseSuccess(groupExten, DcmipecMajorVersion,
    //                             DcmiSpecMinorVersion, paramRevision,
    //                             averageTimePeriods,
    //                             averageTimePeriodDuration,
    //                             averageTimePeriodDurationUnits);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Power Reading
 *
 * @param mode - Statistic Mode
 * @param averageTimePeriods - Supported Rolling Average Time Periods
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t,  // Group Extension Identification
              uint16_t, // Current Power
              uint16_t, // Minimum Power
              uint16_t, // Maximum Power
              uint16_t, // Average Power
              uint32_t, // IPMI Time Stamp
              uint32_t, // Statistics Reporting Time Period
              uint6_t,  // Reserved
              uint1_t,  // Power Reading State
              uint1_t>  // Reserved
    getPowerReading(uint8_t mode, uint8_t averageTimePeriods,
                    uint8_t /* reserved */)
{
    // TODO - Prepare proper handler implementation
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint8_t groupExten = 0;
    // uint16_t currentPower = 0;
    // uint16_t minimumPower = 0;
    // uint16_t maximumPower = 0;
    // uint16_t averagePower = 0;
    // uint32_t timestamp = 0;
    // uint32_t reportingTimePeriod = 0;
    // uint1_t powerReadingState = 0;
    //
    // return ipmi::responseSuccess(groupExten, currentPower, minimumPower,
    //                             maximumPower, averagePower, timestamp,
    //                             reportingTimePeriod, uint6_t(0) reserved,
    // powerReadingState, uint1_t(0) reserved);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Get Power Limit
 *
 * @param groupExten - Group Extension Identification
 * @param reserved
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t,  // Group Extension Identification
              uint16_t, // reserved
              uint8_t,  // Exception Actions
              uint16_t, // Requested Power Limit
              uint32_t, // Correction Time
              uint16_t, // reserved
              uint16_t> // Statistics Sampling Period
    getPowerLimit(uint8_t groupExten, uint16_t /* reserved */)
{
    // TODO - Prepare proper handler implementation
    //
    // Below are variables declared and sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // uint8_t exceptionActions = 0;
    // uint16_t powerLimit = 0;
    // uint32_t correctionTime = 0;
    // uint16_t statSamplingPeriod = 0;
    //
    // return ipmi::responseSuccess(groupExten, uint16_t(0) reserved,
    //                             exceptionActions, powerLimit, correctionTime,
    //                             uint16_t(0) reserved, statSamplingPeriod);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Set Power Limit
 *
 * @param groupExten - Group Extension Identification
 * @param reserved
 * @param exceptionActions
 * @param powerLimit
 * @param correctionTime
 * @param reserved
 * @param statSamplingPeriod
 *
 * @returns IPMI response
 **/

ipmi::RspType<uint8_t> // Group Extension Identification
    setPowerLimit(uint8_t groupExten, uint24_t /* reserved */,
                  uint8_t exceptionActions, uint16_t powerLimit,
                  uint32_t correctionTime, uint16_t /* reserved */,
                  uint16_t statSamplingPeriod)
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess(groupExten);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Activate/Deactivate Power Limit
 *
 * @param groupExten - Group Extension Identification
 * @param powerLimitActivation
 * @param reserved
 *
 *  @returns IPMI response
 **/

ipmi::RspType<uint8_t> // Group Extension Identification
    activatePowerLimit(uint8_t groupExten, uint8_t powerLimitActivation)
{
    // TODO - Prepare proper handler implementation
    //
    // Below is sucessful response with proper params,
    // that can be used within handler implementation.
    //
    // return ipmi::responseSuccess(groupExten);
    LOG_ENTRY;
    return ipmi::responseInvalidCommand();
}

/**
 * @brief Registration of Node Manager IPMI commands handlers
 **/

void registerNmIpmiFunctions(void)
{
    LOGGER_INFO << "Registering Node Manager IPMI commands";

    auto ret = sd_bus_error_add_map(kNodeManagerDBusErrors);
    if (ret < 0)
    {
        LOGGER_CRIT << "Adding the NodeManager Error map has failed, errno:"
                    << ret;
    }

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdEnableNmPolicyCtrl,
                             ipmi::Privilege::Admin, enableNmPolicyControl);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdSetNmPolicy,
                             ipmi::Privilege::Admin, setNmPolicy);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdGetNmPolicy,
                             ipmi::Privilege::User, getNodeManagerPolicy);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdSetNodeManagerPolicyAlertThresholds,
    //    ipmi::Privilege::Admin, setNodeManagerPolicyAlertThresholds);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdGetNodeManagerPolicyAlertThresholds,
    //    ipmi::Privilege::User, getNodeManagerPolicyAlertThresholds);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdSetNodeManagerPolicySuspendPeriods,
    //    ipmi::Privilege::Admin, setNodeManagerPolicySuspendPeriods);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdGetNodeManagerPolicySuspendPeriods,
    //    ipmi::Privilege::User, getNodeManagerPolicySuspendPeriods);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdResetNmStat,
                             ipmi::Privilege::Admin, resetNmStatistics);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdGetNmStat,
                             ipmi::Privilege::User, getNmStatistics);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdGetNmCapab,
                             ipmi::Privilege::User, getNmCapabilities);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdGetNmVersion,
                             ipmi::Privilege::User, getNmVersion);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdSetNmPowerDrawRange,
                             ipmi::Privilege::Admin, setNmPowerDrawRange);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdSetTurboSyncRatio,
    //    ipmi::Privilege::Admin, setTurboSynchronizationRatio);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdGetTurboSyncRatio,
    //    ipmi::Privilege::User, getTurboSynchronizationRatio);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdSetTotalPowerBudget,
                             ipmi::Privilege::Admin, setTotalPowerBudget);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdGetTotalPowerBudget,
                             ipmi::Privilege::User, getTotalPowerBudget);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdSetMaxAllowedCPUPStateThreads,
    //    ipmi::Privilege::Admin, setMaxAllowedCPUPStateThreads);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdGetMaxAllowedCPUPStateThreads,
    //    ipmi::Privilege::User, getMaxAllowedCPUPStateThreads);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdGetNumberOfCPUPStateThreads,
    //    ipmi::Privilege::User, getNumberOfCPUPStateThreads);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdSetHWProtectionCoefficient,
    //    ipmi::Privilege::Admin, setHWProtectionCoefficient)

    // TODO - Uncomment after command handler implementation
    // ipmi::registerOemHandler(
    //    ipmi::prioOemBase, oem::intelOemNumber,
    //    intel::general::cmdGetHWProtectionCoefficient,
    //    ipmi::Privilege::User, getHWProtectionCoefficient);

    ipmi::registerOemHandler(ipmi::prioOemBase, intel::oem::intelOemNumber,
                             intel::general::cmdGetLimitingPolicyId,
                             ipmi::Privilege::User, getLimitingPolicyId);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerHandler(ipmi::prioOemBase, intel::netFnDCMI,
    //                      intel::DCMI::cmdGetDCMICapabilityInfo,
    //                      ipmi::Privilege::User, getDCMICapabilityInfo);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerHandler(ipmi::prioOemBase, intel::netFnDCMI,
    //                      intel::DCMI::cmdGetPowerReading,
    //                      ipmi::Privilege::User, getPowerReading);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerHandler(ipmi::prioOemBase, intel::netFnDCMI,
    //                      intel::DCMI::cmdGetPowerLimit,
    //                      ipmi::Privilege::User, getPowerLimit);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerHandler(ipmi::prioOemBase, intel::netFnDCMI,
    //                      intel::DCMI::cmdSetPowerLimit,
    //                      ipmi::Privilege::Admin, setPowerLimit);

    // TODO - Uncomment after command handler implementation
    // ipmi::registerHandler(ipmi::prioOemBase, intel::netFnDCMI,
    //                      intel::DCMI::cmdActivatePowerLimit,
    //                      ipmi::Privilege::Admin, activatePowerLimit);
}
} // namespace nmipmi
