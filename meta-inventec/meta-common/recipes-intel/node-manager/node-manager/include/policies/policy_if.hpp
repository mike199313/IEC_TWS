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

#include "flow_control.hpp"
#include "policy_basic_if.hpp"
#include "policy_types.hpp"
#include "utility/dbus_enable_if.hpp"

namespace nodemanager
{

class PolicyIf : public PolicyBasicIf, public RunnerIf
{
  public:
    PolicyIf(DbusState dbusState) : PolicyBasicIf(dbusState)
    {
    }
    virtual ~PolicyIf() = default;

    /**
     * @brief Verifies given parameters and other requirement for policy
     * operation
     *
     * @param params
     */
    virtual void verifyPolicy(const PolicyParams& params) const = 0;

    /**
     * @brief Verifies if parameters are correct and throws proper error
     *
     * @param params
     */
    virtual void verifyParameters(const PolicyParams& params) const = 0;

    /**
     * @brief Updates policy parameters
     *
     * @param params
     */
    virtual void updateParams(const PolicyParams& params) = 0;

    /**
     * @brief For parameters that can be fixed, changes them to proper bounds.
     * To be used during policy initialization.
     *
     */
    virtual void adjustCorrectableParameters() = 0;

    /**
     * @brief Verifies current policy params, corrects correctable errors,
     * switches state to/from suspended depending on errors in parameters
     *
     */
    virtual void validateParameters() = 0;

    /**
     * @brief Should be called after policy is created and parameters are set.
     *
     */
    virtual void postCreate() = 0;

    /**
     * @brief Get the limit value
     *
     * @return uint16_t
     */
    virtual uint16_t getLimit() const = 0;

    /**
     * @brief Set the Limit value
     *
     * @param limit
     */
    virtual void setLimit(uint16_t value) = 0;

    /**
     * @brief Set state to selected
     *
     * @param isLimitSelected
     */
    virtual void setLimitSelected(bool isLimitSelected) = 0;

    /**
     * @brief Get the Component Id assigned to the policy
     *
     * @return DeviceIndex
     */
    virtual DeviceIndex getComponentId() const = 0;

    /**
     * @brief Get the Internal Component Id assigned to the policy
     *
     * @return DeviceIndex
     */
    virtual DeviceIndex getInternalComponentId() const = 0;

    /**
     * @brief Get the Strategy assigned to the policy
     *
     * @return BudgetingStrategy
     */
    virtual BudgetingStrategy getStrategy() const = 0;
};

} // namespace nodemanager
