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
#include "nm_dbus_const.hpp"
#include "nm_logger.hpp"

#include <optional>
#include <ratio>
#include <regex>
#include <string>

namespace nmipmi
{

const static constexpr uint8_t kComponentIdAll = 255;
const static constexpr uint8_t kPolicyOwnerTotalBudget = 254;
const static constexpr uint3_t kPolicyTypePowerControl = 1;
const static constexpr uint4_t kGlobalStatsDomainId = 0;
const static constexpr uint8_t kNodeManagerMaxPolicies = 64;

using StatGlobalResponse = ipmi::RspType<uint16_t, // Current Value
                                         uint16_t, // Minimum Value
                                         uint16_t, // Maximum Value
                                         uint16_t, // Average Value
                                         uint32_t, // Timestamp
                                         uint32_t, // Statistic Reporting Period
                                         uint4_t,  // Domain ID
                                         uint1_t,  // AdministrativeState
                                         uint1_t,  // OperationalState
                                         uint1_t,  // MeasurementState
                                         uint1_t>;

using StatDomainEnergyResponse =
    ipmi::RspType<uint64_t, // Accumulated Energy Value
                  uint32_t, // Timestamp
                  uint32_t, // Statistic Reporting Period
                  uint4_t,  // Domain ID
                  uint1_t,  // AdministrativeState
                  uint1_t,  // OperationalState
                  uint1_t,  // MeasurementState
                  uint1_t>;

using StatDomainResponse = ipmi::RspType<uint16_t, // Current Value
                                         uint16_t, // Minimum Value
                                         uint16_t, // Maximum Value
                                         uint16_t, // Average Value
                                         uint32_t, // Timestamp
                                         uint32_t, // Statistic Reporting Period
                                         uint4_t,  // Domain ID
                                         uint1_t,  // AdministrativeState
                                         uint1_t,  // OperationalState
                                         uint1_t,  // MeasurementState
                                         uint1_t>;

using StatPolicyResponse = ipmi::RspType<uint16_t, // Current Value
                                         uint16_t, // Minimum Value
                                         uint16_t, // Maximum Value
                                         uint16_t, // Average Value
                                         uint32_t, // Timestamp
                                         uint32_t, // Statistic Reporting Period
                                         uint4_t,  // Domain ID
                                         uint1_t,  // AdministrativeState
                                         uint1_t,  // OperationalState
                                         uint1_t,  // MeasurementState
                                         uint1_t>;

/**
 * @brief converts NM dbus error into ipmi ipmi::Cc in context of given cmd
 */
ipmi::Cc getCc(ipmi::Cmd cmd, boost::system::error_code ec)
{
    LOGGER_DEBUG << "Get response code nm for ipmi cmd, error: " << ec.message()
                 << ", err_code:" << ec.value()
                 << ", ipmi_cmd:" << unsigned{cmd};
    if (!ec)
    {
        return ipmi::ccSuccess;
    }
    if (ec.value() == EBADR)
    {
        return ccInvalidPolicyId;
    }
    else
    {
        const auto it = kCmdCcErrnoMap.find(cmd);
        if (it != kCmdCcErrnoMap.cend())
        {
            const auto ccMap = it->second;
            const auto it2 = ccMap.find(ErrorCodes{ec.value()});
            if (it2 != ccMap.cend())
            {
                const auto cc = it2->second;
                return cc;
            }
            else
            {
                LOGGER_ERR << "Unexpected dbus error for ipmi cmd, error: "
                           << ec.message() << ", err_code:" << ec.value()
                           << ", ipmi_cmd:" << unsigned{cmd};
                return ipmi::ccUnspecifiedError;
            }
        }
        else
        {
            LOGGER_ERR << "Unsupported ipmi command!, ipmi_cmd:"
                       << unsigned{cmd};
            return ipmi::ccUnspecifiedError;
        }
    }
}

ipmi::RspType<> deletePolicy(ipmi::Context::ptr ctx, uint4_t domainId,
                             uint8_t policyId)
{

    boost::system::error_code ec;
    NmService nmService(ctx);
    ctx->bus->yield_method_call<void>(
        ctx->yield, ec, nmService.getServiceName(),
        nmService.getPolicyPath(domainId, policyId), kObjectDeleteInterface,
        "Delete");
    const auto cc = getCc(ctx->cmd, ec);
    if (cc)
    {
        return ipmi::response(cc);
    }
    return ipmi::responseSuccess();
}

/**
 * @brief Maps sendAlert and shutdownSystem bits into enum LimitException values
 * used by the NM.
 *  noAction = 0,
 *  powerOff = 1,
 *  logEvent = 2,
 *  logEventAndPowerOff = 3
 */
int limitException(uint1_t sendAlert, uint1_t shutdownSystem)
{
    if (sendAlert)
    {
        return (shutdownSystem) ? 3 : 2;
    }
    else
    {
        return (shutdownSystem) ? 1 : 0;
    }
}

/**
 * @brief Maps limitException into sendAlert and shutdown bits.
 *  Possible limitException values are:
 *  noAction = 0,
 *  powerOff = 1,
 *  logEvent = 2,
 *  logEventAndPowerOff = 3
 */
void parseLimitException(const int& limitException, uint1_t& sendAlert,
                         uint1_t& shutdown)
{
    if (limitException == 0)
    {
        sendAlert = 0;
        shutdown = 0;
    }
    else if (limitException == 1)
    {
        sendAlert = 0;
        shutdown = 1;
    }
    else if (limitException == 2)
    {
        sendAlert = 1;
        shutdown = 0;
    }
    else if (limitException == 3)
    {
        sendAlert = 1;
        shutdown = 1;
    }
}

/**
 * @Brief creates an ipmi reponse whenfor case whn domainId cannot be found.
 */
ipmi::RspType<uint4_t, // Next Valid domain ID
              uint4_t, // reserved
              uint8_t  // Number of domains
              >
    responseNextDomainId(std::map<uint4_t, std::set<uint8_t>> dpMap, uint4_t id)
{
    std::set<decltype(id)> keys;
    std::transform(dpMap.begin(), dpMap.end(), std::inserter(keys, keys.end()),
                   [](auto pair) { return pair.first; });

    uint4_t nextDomainId = 0;
    auto it = std::upper_bound(keys.cbegin(), keys.cend(), id);
    if (it != keys.cend())
    {
        nextDomainId = *it;
    }

    LOGGER_INFO << "Reqested domainId does not exist, domainId: "
                << static_cast<unsigned>(id)
                << ", next domainId:" << static_cast<unsigned>(nextDomainId);

    return ipmi::response(ccInvalidDomainId, nextDomainId, uint4_t{0},
                          keys.size());
}

/**
 * @Brief creates an ipmi reponse for case when the policyId cannot be found.
 */
ipmi::RspType<uint8_t, // Next Valid Policy ID
              uint8_t  // Number of Policies
              >
    responseNextPolicyId(std::set<uint8_t> policySet, uint8_t id)
{

    uint8_t nextPolicyId = 0;
    auto it = std::upper_bound(policySet.cbegin(), policySet.cend(), id);
    if (it != policySet.cend())
    {
        nextPolicyId = *it;
    }

    LOGGER_INFO << "Reqested policyId does not exist, policyId:" << unsigned{id}
                << ", next policyId:" << unsigned{nextPolicyId};

    return ipmi::response(ccInvalidPolicyId, nextPolicyId, policySet.size());
}
struct NmStatistics
{
    uint16_t currentValue;
    uint16_t minValue;
    uint16_t maxValue;
    uint16_t avgValue;
    uint32_t statReportingPeriod;
    uint32_t timestamp;
};

using StatisticsValues =
    std::unordered_map<std::string,
                       std::variant<double, uint32_t, uint64_t, bool>>;
using AllStatisticsValues = std::unordered_map<std::string, StatisticsValues>;

bool parseStatValuesMap(const AllStatisticsValues& allStatsMap,
                        const std::string& statType, NmStatistics& out,
                        uint1_t& measurementState)
{
    measurementState = 0;
    try
    {
        const StatisticsValues statMap = allStatsMap.at(statType);

        // If there are NaNs it means there is problem with readings
        if (std::isnan(std::get<double>(statMap.at("Max"))) ||
            std::isnan(std::get<double>(statMap.at("Min"))) ||
            std::isnan(std::get<double>(statMap.at("Average"))) ||
            std::isnan(std::get<double>(statMap.at("Current"))))
        {
            out.maxValue = 0;
            out.minValue = 0;
            out.avgValue = 0;
            out.currentValue = 0;
            out.statReportingPeriod = 0;
            out.timestamp = tryCast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
            return true;
        }

        out.maxValue =
            tryCast<uint16_t>(round(std::get<double>(statMap.at("Max"))));
        out.minValue =
            tryCast<uint16_t>(round(std::get<double>(statMap.at("Min"))));
        out.avgValue =
            tryCast<uint16_t>(round(std::get<double>(statMap.at("Average"))));
        out.currentValue =
            tryCast<uint16_t>(round(std::get<double>(statMap.at("Current"))));
        out.statReportingPeriod =
            std::get<uint32_t>(statMap.at("StatisticsReportingPeriod"));
        out.timestamp = tryCast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
        measurementState =
            tryCast<uint1_t>(std::get<bool>(statMap.at("MeasurementState")));

        return true;
    }
    catch (const std::exception& e)
    {
        LOGGER_ERR << "Error while parsing statistics attributes, exception: "
                   << e.what();
        return false;
    }
}

enum class StatsMode
{
    power = 0x01,
    inletTemp = 0x02,
    throttling = 0x03,
    volumetricAirflow = 0x04,
    outletTemp = 0x05,
    chassisPowe = 0x06,
    energyAccumulator = 0x08,
    perPolicyPower = 0x11,
    perPolicyTrigger = 0x12,
    perPolicyThrottling = 0x13,
};

static const std::map<uint5_t, std::string> statModeToDBusMap = {
    {types::enum_cast<uint5_t>(StatsMode::power), "Power"},
    {types::enum_cast<uint5_t>(StatsMode::inletTemp), "Inlet temperature"},
    {types::enum_cast<uint5_t>(StatsMode::throttling), "Throttling"},
    {types::enum_cast<uint5_t>(StatsMode::volumetricAirflow),
     "Volumetric airflow"},
    {types::enum_cast<uint5_t>(StatsMode::outletTemp), "Outlet temperature"},
    {types::enum_cast<uint5_t>(StatsMode::chassisPowe), "Chassis power"},
    {types::enum_cast<uint5_t>(StatsMode::perPolicyPower), "Power"},
    {types::enum_cast<uint5_t>(StatsMode::perPolicyTrigger), "Trigger"},
    {types::enum_cast<uint5_t>(StatsMode::perPolicyThrottling), "Throttling"},
    {types::enum_cast<uint5_t>(StatsMode::energyAccumulator),
     "Energy accumulator"},
};

void policyStateToBitFlags(const int state, uint1_t& operationalState,
                           uint1_t& activationState)
{
    static constexpr int kPolicyStatePending = 0;
    static constexpr int kPolicyStateDisabled = 1;
    static constexpr int kPolicyStateSelected = 4;
    static constexpr int kPolicyStateSuspended = 5;

    activationState = 0;
    operationalState = 1;
    switch (state)
    {
        case kPolicyStateSelected:
            activationState = 1;
            break;
        case kPolicyStatePending:
        case kPolicyStateDisabled:
        case kPolicyStateSuspended:
            operationalState = 0;
            break;
        default:
            break;
    }
}

StatPolicyResponse responsePolicyStat(ipmi::Context::ptr ctx,
                                      const std::string& statType,
                                      uint4_t domainId, uint8_t policyId)
{
    uint1_t administrativeState = 0;
    uint1_t operationalState = 0;
    uint1_t measurementState = 0;
    uint1_t activationState = 0;

    NmService nmService(ctx);
    int policyState = 0;
    boost::system::error_code ec;
    // get policyState
    ec =
        getDbusProperty(ctx, nmService.getServiceName(),
                        nmService.getPolicyPath(domainId, policyId),
                        kPolicyAttributesInterface, "PolicyState", policyState);
    if (ec)
    {
        LOGGER_ERR << "Failed to get PolicyState properties, error:"
                   << ec.message();
        return ipmi::responseUnspecifiedError();
    }
    policyStateToBitFlags(policyState, operationalState, activationState);

    // get enablement status
    const std::optional<bool> policyEn =
        nmService.isPolicyEnabled(domainId, policyId);
    const std::optional<bool> domainEn = nmService.isDomainEnabled(domainId);
    const std::optional<bool> nmEn = nmService.isNmEnabled();
    if (policyEn && domainEn && nmEn)
    {
        administrativeState = (*nmEn && *domainEn && *policyEn) ? 1 : 0;
    }
    else
    {
        LOGGER_ERR << "Cannot get Enabled state";
        return ipmi::responseUnspecifiedError();
    }

    // get statistics
    ec.clear();
    NmStatistics nmStatistics;

    const AllStatisticsValues allStats =
        ctx->bus->yield_method_call<AllStatisticsValues>(
            ctx->yield, ec, nmService.getServiceName(),
            nmService.getPolicyPath(domainId, policyId), kStatisticsInterface,
            "GetStatistics");
    if (const ipmi::Cc cc = getCc(ctx->cmd, ec))
    {
        return ipmi::response(cc);
    }

    if (allStats.find(statType) == allStats.end())
    {
        return ipmi::response(ccInvalidMode);
    }

    if (!parseStatValuesMap(allStats, statType, nmStatistics, measurementState))
    {
        LOGGER_ERR << "Cannot parse statistics";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(
        nmStatistics.currentValue, nmStatistics.minValue, nmStatistics.maxValue,
        nmStatistics.avgValue, nmStatistics.timestamp,
        nmStatistics.statReportingPeriod, domainId, administrativeState,
        operationalState, measurementState, activationState);
}

StatDomainResponse responseDomainStat(ipmi::Context::ptr ctx,
                                      const std::string& statType,
                                      uint4_t domainId)
{
    uint1_t administrativeState = 0;
    uint1_t operationalState = 0;
    uint1_t measurementState = 0;
    uint1_t activationState = 0;

    NmService nmService(ctx);
    // get enablement status
    if (const std::optional<bool> domainEn =
            nmService.isDomainEnabled(domainId))
    {
        administrativeState = (*domainEn) ? 1 : 0;
    }
    else
    {
        LOGGER_ERR << "Cannot get domain Enabled state";
        return ipmi::responseUnspecifiedError();
    };

    // getting statistics
    boost::system::error_code ec;
    NmStatistics nmStatistics;
    const AllStatisticsValues allStats =
        ctx->bus->yield_method_call<AllStatisticsValues>(
            ctx->yield, ec, nmService.getServiceName(),
            nmService.getDomainPath(domainId), kStatisticsInterface,
            "GetStatistics");
    if (const ipmi::Cc cc = getCc(ctx->cmd, ec))
    {
        return ipmi::response(cc);
    }

    if (allStats.find(statType) == allStats.end())
    {
        return ipmi::response(ccInvalidMode);
    }

    if (!parseStatValuesMap(allStats, statType, nmStatistics, measurementState))
    {
        LOGGER_ERR << "Cannot parse statistics";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(
        nmStatistics.currentValue, nmStatistics.minValue, nmStatistics.maxValue,
        nmStatistics.avgValue, nmStatistics.timestamp,
        nmStatistics.statReportingPeriod, domainId, administrativeState,
        operationalState, measurementState, activationState);
}

StatDomainEnergyResponse responseDomainEnergyStat(ipmi::Context::ptr ctx,
                                                  const std::string& statType,
                                                  uint4_t domainId)
{
    uint1_t administrativeState = 0;
    uint1_t operationalState = 0;
    uint1_t measurementState = 0;
    uint1_t activationState = 0;
    uint64_t accumulatedEnergyValue = 0;
    uint32_t statReportingPeriod = 0;
    uint32_t timestamp = 0;

    NmService nmService(ctx);
    // get enablement status
    if (const std::optional<bool> domainEn =
            nmService.isDomainEnabled(domainId))
    {
        administrativeState = (*domainEn) ? 1 : 0;
    }
    else
    {
        LOGGER_ERR << "Cannot get domain Enabled state";
        return ipmi::responseUnspecifiedError();
    };

    // getting statistics
    boost::system::error_code ec;
    const AllStatisticsValues allStats =
        ctx->bus->yield_method_call<AllStatisticsValues>(
            ctx->yield, ec, nmService.getServiceName(),
            nmService.getDomainPath(domainId), kStatisticsInterface,
            "GetStatistics");
    if (const ipmi::Cc cc = getCc(ctx->cmd, ec))
    {
        return ipmi::response(cc);
    }

    if (allStats.find(statType) == allStats.end())
    {
        return ipmi::response(ccInvalidMode);
    }

    try
    {
        const StatisticsValues statMap = allStats.at(statType);
        if (std::isnan(std::get<uint64_t>(statMap.at("Current"))))
        {
            LOGGER_WARN << "Value 'Current' in statistics are NaN";
            timestamp = tryCast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
        }
        else
        {
            accumulatedEnergyValue = std::get<uint64_t>(statMap.at("Current"));

            statReportingPeriod =
                std::get<uint32_t>(statMap.at("StatisticsReportingPeriod"));
            timestamp = tryCast<uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
            measurementState = 1;
        }
    }
    catch (const std::exception& e)
    {
        LOGGER_ERR << "Error while parsing statistics attributes, exception: "
                   << e.what();
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(accumulatedEnergyValue, timestamp,
                                 statReportingPeriod, domainId,
                                 administrativeState, operationalState,
                                 measurementState, activationState);
}

StatGlobalResponse responseGlobalStat(ipmi::Context::ptr ctx,
                                      const std::string& statType)
{
    uint1_t administrativeState = 0;
    uint1_t operationalState = 0;
    uint1_t measurementState = 0;
    uint1_t activationState = 0;

    NmService nmService(ctx);
    // get enablement status
    if (const std::optional<bool> en = nmService.isNmEnabled())
    {
        administrativeState = (*en) ? 1 : 0;
    }
    else
    {
        LOGGER_ERR << "Cannot get Node Manager Enabled state";
        return ipmi::responseUnspecifiedError();
    };

    // getting statistics
    boost::system::error_code ec;
    NmStatistics nmStatistics;
    const AllStatisticsValues allStats =
        ctx->bus->yield_method_call<AllStatisticsValues>(
            ctx->yield, ec, nmService.getServiceName(), nmService.getRootPath(),
            kStatisticsInterface, "GetStatistics");
    if (const ipmi::Cc cc = getCc(ctx->cmd, ec))
    {
        return ipmi::response(cc);
    }
    if (!parseStatValuesMap(allStats, statType, nmStatistics, measurementState))
    {
        LOGGER_ERR << "Cannot parse statistics";
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(
        nmStatistics.currentValue, nmStatistics.minValue, nmStatistics.maxValue,
        nmStatistics.avgValue, nmStatistics.timestamp,
        nmStatistics.statReportingPeriod, uint4_t{0} /*default domain id */,
        administrativeState, operationalState, measurementState,
        activationState);
}

/**
 * @brief Is domain compound
 *        This function is used to determine if given domain is compound or
 *        not (it is simple).
 *
 * @param domainId
 *
 * @returns Returns True if domain is compound and False if not.
 **/
bool isDomainCompound(uint8_t domainId)
{
    if (domainId == 0 || domainId == 5)
    {
        return true;
    }

    return false;
}

std::optional<std::vector<uint8_t>>
    getDomainAvailableComponents(ipmi::Context::ptr ctx, uint4_t domainId)
{
    std::vector<uint8_t> availableComponents;
    NmService nmService(ctx);
    boost::system::error_code ec;

    ec = getDbusProperty(
        ctx, nmService.getServiceName(), nmService.getDomainPath(domainId),
        kDomainAttributesInterface, "AvailableComponents", availableComponents);
    if (ec)
    {
        LOGGER_ERR << "Failed to get AvailableComponents property, error:"
                   << ec.message();
        return std::nullopt;
    }

    return availableComponents;
}

/**
 * @brief Check is given `policyId` is in valid policy range.
 * The policy id is in valid policy range when:
 * - Total number of all policies is below predefined limit,
 * - the policyId is not already present
 *
 * @tparam PolicyId
 * @param ctx
 * @param policyId
 * @return true
 * @return false
 */
template <class PolicyId>
bool isPolicyIdInRange(ipmi::Context::ptr ctx, PolicyId policyId)
{
    NmService nmService(ctx);
    std::map<uint4_t, std::set<uint8_t>> domainPolicyMap =
        nmService.getDomainPolicyMap();
    std::set<uint8_t> allPolicyIds;
    std::for_each(domainPolicyMap.cbegin(), domainPolicyMap.cend(),
                  [&](auto const& m) {
                      allPolicyIds.insert(m.second.begin(), m.second.end());
                  });

    if (allPolicyIds.size() >= kNodeManagerMaxPolicies)
    {
        LOGGER_INFO << "Max limit of Policies was reached";
        return false;
    }
    if (std::find(allPolicyIds.cbegin(), allPolicyIds.cend(), policyId) !=
        allPolicyIds.cend())
    {
        LOGGER_INFO << "Policy Id is not unique";
        return false;
    }
    return true;
}

namespace intel
{

#pragma pack(push, 1)
typedef struct
{
    std::string platform;
    uint8_t major;
    uint8_t minor;
    uint32_t buildNo;
    std::string openbmcHash;
    std::string metaHash;
} MetaRevision;
#pragma pack(pop)

static constexpr const char* versionPurposeBMC =
    "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";
static constexpr const char* softwareFunctionalPath =
    "/xyz/openbmc_project/software/functional";
static constexpr const char* associationIntf =
    "xyz.openbmc_project.Association";
static constexpr const char* softwareActivationIntf =
    "xyz.openbmc_project.Software.Activation";
static constexpr const char* softwareVerIntf =
    "xyz.openbmc_project.Software.Version";

/**
 * @brief Returns the functional firmware version information.
 *
 * It reads the active firmware versions by checking functional
 * endpoints association and matching the input version purpose string.
 * ctx[in]                - ipmi context.
 * reqVersionPurpose[in]  - Version purpose which need to be read.
 * version[out]           - Output Version string.
 *
 * @return Returns '0' on success and '-1' on failure.
 *
 */
int getActiveSoftwareVersionInfo(ipmi::Context::ptr ctx,
                                 const std::string& reqVersionPurpose,
                                 std::string& version)
{
    std::vector<std::string> activeEndPoints;
    boost::system::error_code ec = ipmi::getDbusProperty(
        ctx, ipmi::MAPPER_BUS_NAME, softwareFunctionalPath, associationIntf,
        "endpoints", activeEndPoints);
    if (ec)
    {
        LOGGER_ERR << "Failed to get Active firmware version endpoints";
        return -1;
    }

    for (auto& activeEndPoint : activeEndPoints)
    {
        std::string serviceName;
        ec = ipmi::getService(ctx, softwareActivationIntf, activeEndPoint,
                              serviceName);
        if (ec)
        {
            LOGGER_ERR << "Failed to perform getService, objPath:"
                       << activeEndPoint;
            continue;
        }

        ipmi::PropertyMap propMap;
        ec = ipmi::getAllDbusProperties(ctx, serviceName, activeEndPoint,
                                        softwareVerIntf, propMap);
        if (ec)
        {
            LOGGER_ERR
                << "Failed to perform GetAll on Version interface, service:"
                << serviceName << ", path:" << activeEndPoint;
            continue;
        }

        std::string* purposeProp =
            std::get_if<std::string>(&propMap["Purpose"]);
        std::string* versionProp =
            std::get_if<std::string>(&propMap["Version"]);
        if (!purposeProp || !versionProp)
        {
            LOGGER_ERR << "Failed to get version or purpose property";
            continue;
        }

        // Check for requested version information and return if found.
        if (*purposeProp == reqVersionPurpose)
        {
            version = *versionProp;
            return 0;
        }
    }
    LOGGER_INFO << "Failed to find version information, purpose:"
                << reqVersionPurpose;
    return -1;
}

// Support both 2 solutions:
// 1.Current solution  2.7.0-dev-533-g14dc00e79-5e7d997
//   openbmcTag  2.7.0-dev
//   BuildNo     533
//   openbmcHash 14dc00e79
//   MetaHasg    5e7d997
//
// 2.New solution  wht-0.2-3-gab3500-38384ac
//   IdStr        wht
//   Major        0
//   Minor        2
//   buildNo      3
//   MetaHash     ab3500
//   openbmcHash  38384ac
std::optional<MetaRevision> convertIntelVersion(std::string& s)
{
    std::smatch results;
    MetaRevision rev;
    std::regex pattern1("(\\d+?).(\\d+?).\\d+?-\\w*?-(\\d+?)-g(\\w+?)-(\\w+?)");
    constexpr size_t matchedPhosphor = 6;
    if (std::regex_match(s, results, pattern1))
    {
        if (results.size() == matchedPhosphor)
        {
            rev.platform = "whtref";
            rev.major = static_cast<uint8_t>(std::stoi(results[1]));
            rev.minor = static_cast<uint8_t>(std::stoi(results[2]));
            rev.buildNo = static_cast<uint32_t>(std::stoi(results[3]));
            rev.openbmcHash = results[4];
            rev.metaHash = results[5];
            std::string versionString =
                rev.platform + ":" + std::to_string(rev.major) + ":" +
                std::to_string(rev.minor) + ":" + std::to_string(rev.buildNo) +
                ":" + rev.openbmcHash + ":" + rev.metaHash;
            LOGGER_INFO << "Get BMC version: " << versionString;
            return rev;
        }
    }
    constexpr size_t matchedIntel = 7;
    std::regex pattern2("(\\w+?)-(\\d+?).(\\d+?)-(\\d+?)-g(\\w+?)-(\\w+?)");
    if (std::regex_match(s, results, pattern2))
    {
        if (results.size() == matchedIntel)
        {
            rev.platform = results[1];
            rev.major = static_cast<uint8_t>(std::stoi(results[2]));
            rev.minor = static_cast<uint8_t>(std::stoi(results[3]));
            rev.buildNo = static_cast<uint32_t>(std::stoi(results[4]));
            rev.openbmcHash = results[6];
            rev.metaHash = results[5];
            std::string versionString =
                rev.platform + ":" + std::to_string(rev.major) + ":" +
                std::to_string(rev.minor) + ":" + std::to_string(rev.buildNo) +
                ":" + rev.openbmcHash + ":" + rev.metaHash;
            LOGGER_INFO << "Get BMC version: " << versionString;
            return rev;
        }
    }

    return std::nullopt;
}

} // namespace intel

static const std::unordered_map<uint4_t, std::string> kTriggerTypeNames = {
    {0, "AlwaysOn"},
    {1, "InletTemperature"},
    {2, "MissingReadingsTimeout"},
    {3, "TimeAfterHostReset"},
    {6, "GPIO"},
    {7, "CPUUtilization"},
    {8, "HostReset"},
    {9, "SMBAlertInterrupt"}};

template <class T>
inline T stringToNumber(const std::unordered_map<T, std::string>& data,
                        const std::string& value)
{
    auto it =
        std::find_if(data.begin(), data.end(), [&value](const auto& item) {
            return item.second == value;
        });
    if (it == data.end())
    {
        throw std::out_of_range("Value is not in range of enum");
    }
    return it->first;
}

template <class T>
inline std::string
    numberToString(const std::unordered_map<T, std::string>& data,
                   const T& value)
{
    auto it = data.find(value);
    if (it == data.end())
    {
        return "";
    }
    return it->second;
}

std::string numberToTriggerName(uint4_t triggerType)
{
    try
    {
        return numberToString(kTriggerTypeNames, triggerType);
    }
    catch (const std::exception& e)
    {
        return "";
    }
}

} // namespace nmipmi
