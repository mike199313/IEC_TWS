/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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
#include "nm_cc.hpp"
#include "nm_dbus_const.hpp"
#include "nm_logger.hpp"

#include <filesystem>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>

namespace nmipmi
{

constexpr static auto kMaxSearchDepth = 4;
static constexpr uint4_t kHwProtectionDomainId{3};
static constexpr uint4_t kCpuPerformanceDomainId{6};

/**
 * @brief Returns true if the 'val' conversion to type K is safe.
 */
template <class K, class T>
bool isCastSafe(const T& val)
{
    if (std::isnan(val))
    {
        return std::numeric_limits<K>::has_quiet_NaN;
    }
    return (std::numeric_limits<K>::max() >= val) &&
           (std::numeric_limits<K>::lowest() <= val);
}

template <class K, class T>
K tryCast(const T& b)
{
    if (isCastSafe<K>(b))
    {
        return static_cast<K>(b);
    }
    else
    {
        const auto tmp = std::string("Cannot cast value: ") +
                         std::to_string(b) + std::string(" to narrower type");
        throw std::range_error(tmp);
    }
}

/**
 * @brief Checks if the strNumber holds only digits.
 *
 * @param strNumber
 * @return true - there are only digits in the strNumber.
 * @return false
 */
bool onlyDigits(const std::string& strNumber)
{
    return !strNumber.empty() &&
           std::find_if(strNumber.begin(), strNumber.end(),
                        [](unsigned char c) { return !std::isdigit(c); }) ==
               strNumber.end();
}

static const std::unordered_map<uint4_t, std::string> kDomainIdToNameMap = {
    {0, "ACTotalPlatformPower"},
    {1, "CPUSubsystem"},
    {2, "MemorySubsystem"},
    {3, "HWProtection"},
    {4, "PCIe"},
    {5, "DCTotalPlatformPower"},
    {6, "CPUPerformance"}};

class NmService
{
  public:
    NmService(const NmService&) = delete;
    NmService& operator=(const NmService&) = delete;
    NmService(NmService&&) = delete;
    NmService& operator=(NmService&&) = delete;
    NmService(ipmi::Context::ptr ctxPtr) : ctx(ctxPtr){};
    ~NmService() = default;

    std::string getDomainPath(const uint4_t id) const
    {
        return getRootPath() + "/Domain/" + domainIdToName(id);
    }

    std::string getTriggerPath(const std::string triggerName) const
    {
        return getRootPath() + "/Trigger/" + triggerName;
    }

    std::string getPolicyPath(const uint4_t domainId,
                              const uint8_t policyId) const
    {
        return getRootPath() + "/Domain/" + domainIdToName(domainId) +
               "/Policy/" + std::to_string(policyId);
    }

    std::string getPolicyPath(const uint4_t domainId,
                              const std::string policyId) const
    {
        return getRootPath() + "/Domain/" + domainIdToName(domainId) +
               "/Policy/" + policyId;
    }

    std::string getRootPath() const
    {
        return "/xyz/openbmc_project/NodeManager";
    }

    std::string getServiceName() const
    {
        return "xyz.openbmc_project.NodeManager";
    }

    std::optional<bool> isNmEnabled() const
    {
        return isEnabled(getRootPath());
    }

    std::optional<bool> isDomainEnabled(const uint4_t domainId) const
    {
        return isEnabled(getDomainPath(domainId));
    }

    std::optional<bool> isPolicyEnabled(const uint4_t domainId,
                                        const uint8_t policyId) const
    {
        return isEnabled(getPolicyPath(domainId, policyId));
    }

    std::map<uint4_t, std::set<uint8_t>> getDomainPolicyMap() const
    {
        boost::system::error_code ec;

        ipmi::DbusObjectInfo nmObject;
        ec = ipmi::getDbusObject(ctx, kObjectMapperService, nmObject);
        if (ec)
        {
            LOGGER_ERR << "Failed to perform ipmi::getDbusObject for the "
                          "ObjectMapper interface, err: "
                       << ec.message();
            return {};
        }
        const auto objVect =
            ctx->bus->yield_method_call<std::vector<std::string>>(
                ctx->yield, ec, nmObject.second, nmObject.first,
                kObjectMapperInterface, "GetSubTreePaths", getRootPath(),
                kMaxSearchDepth,
                std::vector(
                    {kDomainAttributesInterface, kPolicyAttributesInterface}));
        if (ec)
        {
            LOGGER_ERR << "Failed to call GetSubTreePaths, err: "
                       << ec.message();
            return {};
        }
        return pathsToTree<uint4_t, uint8_t>(objVect);
    }

    std::optional<std::vector<std::string>> getAllPoliciesPaths() const
    {
        boost::system::error_code ec;

        ipmi::DbusObjectInfo nmObject;
        ec = ipmi::getDbusObject(ctx, kObjectMapperService, nmObject);
        if (ec)
        {
            LOGGER_ERR << "Failed to perform ipmi::getDbusObject for the "
                          "ObjectMapper interface, err: "
                       << ec.message();
            return std::nullopt;
        }
        const auto objVect =
            ctx->bus->yield_method_call<std::vector<std::string>>(
                ctx->yield, ec, nmObject.second, nmObject.first,
                kObjectMapperInterface, "GetSubTreePaths", getRootPath(), 0,
                std::vector({kPolicyAttributesInterface}));
        if (ec)
        {
            LOGGER_ERR << "Failed to call GetSubTreePaths, err: "
                       << ec.message();
            return std::nullopt;
        }
        return objVect;
    }

    /**
     * @brief This function checks if given domainId and policyId are present in
     * NodeManager service.
     * @returns ccInvalidDomainId - if domain is not present
     *          ccInvalidPolicyId - if policy is not present
     *          ccSuccess - if both domainId and policyId are present in dpMap.
     */
    template <class DomainT, class PolicyT>
    ipmi::Cc verifyDomainAndPolicyPresence(const DomainT& domainId,
                                           const PolicyT& policyId) const
    {
        if ((domainId == kCpuPerformanceDomainId) ||
            (domainId == kHwProtectionDomainId))
        {
            return ccInvalidDomainId;
        }

        std::map<uint4_t, std::set<uint8_t>> dpMap = getDomainPolicyMap();
        const auto& it = dpMap.find(domainId);
        if (it == dpMap.cend())
        {
            return ccInvalidDomainId;
        }
        else
        {
            const auto& policyIds = it->second;
            if (policyIds.count(policyId) == 0)
            {
                return ccInvalidPolicyId;
            }
        }
        return ipmi::ccSuccess;
    }

  private:
    std::string domainIdToName(const uint4_t id) const
    {
        auto it = kDomainIdToNameMap.find(id);
        if (it != kDomainIdToNameMap.cend())
        {
            return it->second;
        }
        else
        {
            return "Invalid";
        }
    }

    uint4_t domainNameToId(const std::string name) const
    {
        for (const auto& [dId, dName] : kDomainIdToNameMap)
        {
            if (dName == name)
            {
                return dId;
            }
        }
        throw std::range_error("Unrecognized domain name: " + name);
    }

    std::optional<bool> isEnabled(const std::string& objectPath) const
    {
        boost::system::error_code ec;
        auto enabledFlag = false;
        ec = getDbusProperty(ctx, getServiceName(), objectPath,
                             kObjectEnableInterface, "Enabled", enabledFlag);
        if (ec)
        {
            LOGGER_ERR << "Failed to get Enabled property, err:" << ec.message()
                       << ", objectPath:" << objectPath;
            return std::nullopt;
        }
        return enabledFlag;
    }

    /**
     * @brief This method converts a flat structure of domain -> policy ids into
     * a map.
     * For legacy reasons it will also filter all polices that have IDs not
     * compatibile with PolicyT (only numbers are accepted) Example: giving this
     * as an input:
     *    "/xyz/openbmc_project/NodeManager/Domain/ACTotalPlatformPower"
     *    "/xyz/openbmc_project/NodeManager/Domain/ACTotalPlatformPower/Policy/1"
     *    "/xyz/openbmc_project/NodeManager/Domain/ACTotalPlatformPower/Policy/3"
     *    "/xyz/openbmc_project/NodeManager/Domain/ACTotalPlatformPower/Policy/internal_4"
     *    "/xyz/openbmc_project/NodeManager/Domain/ACTotalPlatformPower/Policy/internal_5"
     *    "/xyz/openbmc_project/NodeManager/Domain/MemorySubsystem"
     * a map of this structure will be returned:
     *    0 -> {1,3}
     *    2 -> {}
     */
    template <class DomainT, class PolicyT>
    std::map<DomainT, std::set<PolicyT>>
        pathsToTree(std::vector<std::string> objectPaths) const
    {
        std::map<DomainT, std::set<PolicyT>> retMap;
        for (const auto& objectPath : objectPaths)
        {
            auto path = std::filesystem::path(objectPath);
            std::filesystem::path pId;
            std::filesystem::path dId;

            // get the domain and policy id from the path
            std::adjacent_find(path.begin(), path.end(),
                               [&pId, &dId](const auto& a, const auto& b) {
                                   if (a.native().compare("Domain") == 0)
                                   {
                                       dId = b;
                                   }
                                   if (a.native().compare("Policy") == 0)
                                   {
                                       pId = b;
                                       return true;
                                   }
                                   return false;
                               });

            // convert domain and policy id to proper types if possible
            if (!dId.empty())
            {
                try
                {
                    const DomainT domainId = domainNameToId(dId);
                    auto& policySet = retMap[domainId];
                    if (!pId.empty() && onlyDigits(pId))
                    {
                        try
                        {
                            const PolicyT policyId =
                                tryCast<PolicyT>(std::stoul(pId));
                            policySet.emplace(policyId);
                        }
                        catch (...)
                        {
                            // do nothing
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    LOGGER_ERR << "Cannot parse domainId or policyId, the "
                                  "objectPath will be skipped, objPath:"
                               << path.native() << ", exception:" << e.what();
                    continue;
                }
            }
        }
        return retMap;
    }

    ipmi::Context::ptr ctx;
};

} // namespace nmipmi