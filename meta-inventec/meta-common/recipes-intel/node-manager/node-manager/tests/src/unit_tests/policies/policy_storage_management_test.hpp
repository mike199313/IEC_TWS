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

#include "policies/policy_storage_management.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class PolicyStorageManagementTest : public ::testing::Test
{
  protected:
    PolicyStorageManagementTest() = default;
    virtual ~PolicyStorageManagementTest() = default;

    virtual void SetUp() override
    {
        std::filesystem::remove_all(policyFilesDir_);
        sut_ = std::make_shared<PolicyStorageManagement>(policyFilesDir_);
    }

    virtual void TearDown()
    {
        std::filesystem::remove_all(policyFilesDir_);
    }

    PolicyParams getTestPolicyParams()
    {
        return {
            125u,
            1024u,
            100u,
            static_cast<PolicyStorage>(9u),
            static_cast<PowerCorrectionType>(11u),
            static_cast<LimitException>(25u),
            {{{"testString00", "testString01"},
              {"testString02",
               std::vector<std::string>{"testString03", "testString04"}}},
             {{"testString05", "testString06"},
              {"testString07",
               std::vector<std::string>{"testString08", "testString09"}}}},
            {{"testString10", {125u, 100u}}},
            2u,
            1050u,
            "testString11",
        };
    }

    std::shared_ptr<PolicyStorageManagement> sut_;
    std::filesystem::path policyFilesDir_ =
        std::filesystem::temp_directory_path() / "policies";
};

TEST_F(PolicyStorageManagementTest, ReadPoliciesEmptyDirReturnEmpty)
{
    std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
        policies = sut_->policiesRead();
    EXPECT_EQ(policies.size(), 0u);
}

TEST_F(PolicyStorageManagementTest, PolicyWriteReadSameData)
{
    PolicyId policyId = "0";
    nlohmann::json policyJson = {{"domainId", DomainId::HwProtection},
                                 {"owner", PolicyOwner::bmc},
                                 {"isEnabled", false},
                                 {"policyParams", getTestPolicyParams()}};
    sut_->policyWrite(policyId, policyJson);

    std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
        policies = sut_->policiesRead();
    auto [policyIdRead, domainIdRead, ownerRead, isEnabledRead,
          policyParamsRead] = policies.at(0);
    EXPECT_EQ(policyId, policyIdRead);
    EXPECT_EQ(DomainId::HwProtection, domainIdRead);
    EXPECT_EQ(PolicyOwner::bmc, ownerRead);
    EXPECT_EQ(false, isEnabledRead);
    EXPECT_EQ(getTestPolicyParams(), policyParamsRead);
}

TEST_F(PolicyStorageManagementTest, PolicyDoubleWriteReadSinglePolicy)
{
    PolicyId policyId = "0";
    PolicyParams policyParams = getTestPolicyParams();
    nlohmann::json policyJson = {{"domainId", DomainId::HwProtection},
                                 {"owner", PolicyOwner::bmc},
                                 {"isEnabled", false},
                                 {"policyParams", policyParams}};
    sut_->policyWrite(policyId, policyJson);
    policyParams.componentId++;
    policyJson = {{"domainId", DomainId::HwProtection},
                  {"owner", PolicyOwner::bmc},
                  {"isEnabled", false},
                  {"policyParams", policyParams}};
    sut_->policyWrite(policyId, policyJson);
    std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
        policies = sut_->policiesRead();
    EXPECT_EQ(policies.size(), 1u);
}

TEST_F(PolicyStorageManagementTest, WriteMultiplePoliciesReadMultiplePolicies)
{
    PolicyId policyId = "0";
    nlohmann::json policyJson = {{"domainId", DomainId::HwProtection},
                                 {"owner", PolicyOwner::bmc},
                                 {"isEnabled", false},
                                 {"policyParams", getTestPolicyParams()}};

    sut_->policyWrite(policyId, policyJson);
    policyId = "1";
    sut_->policyWrite(policyId, policyJson);
    std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
        policies = sut_->policiesRead();
    EXPECT_EQ(policies.size(), 2u);
}

TEST_F(PolicyStorageManagementTest, PolicyDeletePolicyIsDeleted)
{
    PolicyId policyId = "0";
    nlohmann::json policyJson = {{"domainId", DomainId::HwProtection},
                                 {"owner", PolicyOwner::bmc},
                                 {"isEnabled", false},
                                 {"policyParams", getTestPolicyParams()}};

    sut_->policyWrite(policyId, policyJson);
    sut_->policyDelete(policyId);
    std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool, PolicyParams>>
        policies = sut_->policiesRead();
    EXPECT_EQ(policies.size(), 0u);
}