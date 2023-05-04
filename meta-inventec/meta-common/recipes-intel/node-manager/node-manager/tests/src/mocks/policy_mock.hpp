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

#include "policies/policy_if.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class PolicyMock : public PolicyIf
{
  public:
    PolicyMock() : PolicyIf(DbusState::disabled){};
    MOCK_METHOD(void, initialize, (), (override));
    MOCK_METHOD(void, onStateChanged, (), (override));
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(PolicyId, getId, (), (const, override));
    MOCK_METHOD(bool, isParentEnabled, (), (const, override));
    MOCK_METHOD(void, verifyPolicy, (const PolicyParams& params),
                (const, override));
    MOCK_METHOD(void, verifyParameters, (const PolicyParams& params),
                (const, override));
    MOCK_METHOD(void, updateParams, (const PolicyParams& params), (override));
    MOCK_METHOD(void, adjustCorrectableParameters, (), (override));
    MOCK_METHOD(void, validateParameters, (), (override));
    MOCK_METHOD(void, postCreate, (), (override));
    MOCK_METHOD(uint16_t, getLimit, (), (const, override));
    MOCK_METHOD(PolicyState, getState, (), (const, override));
    MOCK_METHOD(uint8_t, getComponentId, (), (const, override));
    MOCK_METHOD(uint8_t, getInternalComponentId, (), (const, override));
    MOCK_METHOD(BudgetingStrategy, getStrategy, (), (const, override));
    MOCK_METHOD(void, installTrigger, (), (override));
    MOCK_METHOD(void, uninstallTrigger, (), (override));
    MOCK_METHOD(void, setLimitSelected, (bool));
    MOCK_METHOD(const std::string&, getShortObjectPath, (), (const));
    MOCK_METHOD(const std::string&, getObjectPath, (), (const));
    MOCK_METHOD(PolicyOwner, getOwner, (), (const));
    MOCK_METHOD(void, setLimit, (uint16_t), (override));
    MOCK_METHOD(bool, isRunning, (), (const, override));
    MOCK_METHOD(bool, isEnabledOnDbus, (), (const, override));
    MOCK_METHOD(bool, isEditable, (), (const, override));
    MOCK_METHOD(nlohmann::json, toJson, (), (const, override));
};
