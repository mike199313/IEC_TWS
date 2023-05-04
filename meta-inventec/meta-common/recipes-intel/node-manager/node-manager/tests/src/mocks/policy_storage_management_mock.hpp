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

using namespace nodemanager;

class PolicyStorageManagementMock : public PolicyStorageManagementIf
{
  public:
    MOCK_METHOD((std::vector<std::tuple<PolicyId, DomainId, PolicyOwner, bool,
                                        PolicyParams>>),
                policiesRead, (), (override));
    MOCK_METHOD(bool, policyWrite,
                (const PolicyId&, const nlohmann::json& policyJson),
                (override));
    MOCK_METHOD(bool, policyDelete, (PolicyId), (override));
};
