
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

#include "policies/policy_factory.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class PolicyFactoryMock : public PolicyFactoryIf
{
  public:
    MOCK_METHOD(std::shared_ptr<PolicyIf>, createPolicy,
                (std::shared_ptr<DomainInfo>&, PolicyType, PolicyId,
                 PolicyOwner, uint16_t, DeleteCallback, DbusState,
                 PolicyEditable, bool, std::shared_ptr<KnobCapabilitiesIf>,
                 const std::vector<std::shared_ptr<ComponentCapabilitiesIf>>&),
                (override));
};
