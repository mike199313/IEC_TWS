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

#include "policy_basic_if.hpp"
#include "policy_types.hpp"
#include "triggers/trigger_enums.hpp"
namespace nodemanager
{

class PolicyStateIf
{
  public:
    virtual ~PolicyStateIf() = default;
    virtual void
        initialize(std::shared_ptr<PolicyIf> policyArg,
                   std::shared_ptr<sdbusplus::asio::connection> busArg) = 0;
    virtual PolicyState getState() const = 0;
    virtual std::unique_ptr<PolicyStateIf>
        onParametersValidation(bool isValid) const = 0;
    virtual std::unique_ptr<PolicyStateIf>
        onEnabled(bool isPolicyEnabled) const = 0;
    virtual std::unique_ptr<PolicyStateIf>
        onParentEnabled(bool isParentEnabled) const = 0;
    virtual std::unique_ptr<PolicyStateIf>
        onTriggerAction(TriggerActionType at) const = 0;
    virtual std::unique_ptr<PolicyStateIf>
        onLimitSelection(bool isLimitSelected) const = 0;
};

} // namespace nodemanager
