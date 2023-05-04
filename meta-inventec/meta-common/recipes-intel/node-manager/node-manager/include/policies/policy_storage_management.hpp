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

#include "config/persistent_storage.hpp"
#include "policy_storage_management_if.hpp"
#include "policy_types.hpp"

namespace nodemanager
{

void to_json(nlohmann::json& j, const DomainId& domainId)
{
    j = enumToStr(kDomainIdNames, domainId);
}

void from_json(const nlohmann::json& j, DomainId& domainId)
{
    domainId = strToEnum<errors::InvalidDomainId>(kDomainIdNames, j);
}

/**
 * @brief Class used to manage policy configuration in storage
 */
class PolicyStorageManagement : public PolicyStorageManagementIf
{
  public:
    PolicyStorageManagement(const PolicyStorageManagement&) = delete;
    PolicyStorageManagement& operator=(const PolicyStorageManagement&) = delete;
    PolicyStorageManagement(PolicyStorageManagement&&) = delete;
    PolicyStorageManagement& operator=(PolicyStorageManagement&&) = delete;

    PolicyStorageManagement(std::filesystem::path policiesDirArg =
                                "/var/lib/node-manager/policies/") :
        policiesDir(policiesDirArg)
    {
    }

    /**
     * @brief Get the Params used to map json fields to this class structures
     *
     * @return tuple of tuples that describe the json mapping.
     */
    auto getParamsToRead(DomainId& domainId, PolicyOwner& owner,
                         bool& isEnabled, PolicyParams& policyParams)
    {
        const auto validate = [](const auto& value) -> bool { return true; };
        return std::make_tuple(
            std::make_tuple(NULL, "domainId", std::ref(domainId), validate),
            std::make_tuple(NULL, "owner", std::ref(owner), validate),
            std::make_tuple(NULL, "isEnabled", std::ref(isEnabled), validate),
            std::make_tuple("policyParams", "correctionInMs",
                            std::ref(policyParams.correctionInMs), validate),
            std::make_tuple("policyParams", "limit",
                            std::ref(policyParams.limit), validate),
            std::make_tuple("policyParams", "statReportingPeriod",
                            std::ref(policyParams.statReportingPeriod),
                            validate),
            std::make_tuple("policyParams", "policyStorage",
                            std::ref(policyParams.policyStorage), validate),
            std::make_tuple("policyParams", "powerCorrectionType",
                            std::ref(policyParams.powerCorrectionType),
                            validate),
            std::make_tuple("policyParams", "limitException",
                            std::ref(policyParams.limitException), validate),
            std::make_tuple("policyParams", "suspendPeriods",
                            std::ref(policyParams.suspendPeriods), validate),
            std::make_tuple("policyParams", "thresholds",
                            std::ref(policyParams.thresholds), validate),
            std::make_tuple("policyParams", "componentId",
                            std::ref(policyParams.componentId), validate),
            std::make_tuple("policyParams", "triggerLimit",
                            std::ref(policyParams.triggerLimit), validate),
            std::make_tuple("policyParams", "triggerType",
                            std::ref(policyParams.triggerType), validate));
    }

    std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
        policiesRead()
    {
        std::vector<
            std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
            readPolicies;
        if (storage.exists(policiesDir))
        {
            for (auto const& entry :
                 std::filesystem::directory_iterator(policiesDir))
            {
                if (entry.path().extension().compare(".json") == 0)
                {
                    PolicyId policyId = policyIdFromPath(entry.path());
                    DomainId domainId;
                    PolicyOwner owner;
                    bool isEnabled;
                    PolicyParams policyParams;
                    if (storage.readJsonFile(entry.path(),
                                             getParamsToRead(domainId, owner,
                                                             isEnabled,
                                                             policyParams)))
                    {
                        readPolicies.push_back(
                            std::make_tuple(policyId, domainId, owner,
                                            isEnabled, policyParams));
                    }
                    else
                    {
                        Logger::log<LogLevel::warning>(
                            "Cannot create policy with id %s from file - "
                            "format is invalid. Removing the file",
                            policyId);
                        policyDelete(policyId);
                    }
                }
            }
            std::sort(readPolicies.begin(), readPolicies.end(),
                      [](const auto& a, const auto& b) {
                          return (std::get<PolicyId>(a) <
                                  std::get<PolicyId>(b));
                      });
        }
        return readPolicies;
    }

    bool policyWrite(const PolicyId& policyId, const nlohmann::json& policyJson)
    {
        return storage.store(policyFilePath(policyId), policyJson);
    }

    bool policyDelete(PolicyId policyId)
    {
        if (storage.exists(policyFilePath(policyId)))
        {
            return storage.remove(policyFilePath(policyId));
        }
        else
        {
            Logger::log<LogLevel::debug>("Trying to delete policy file with id "
                                         "%s but the file does not exist",
                                         policyId);
            return true;
        }
    }

  private:
    PersistentStorage storage;
    std::filesystem::path policiesDir;

    std::filesystem::path policyFilePath(PolicyId policyId)
    {
        return policiesDir / (policyId + ".json");
    }

    PolicyId policyIdFromPath(std::filesystem::path policyFilePath)
    {
        return policyFilePath.stem().string();
    }
};

} // namespace nodemanager