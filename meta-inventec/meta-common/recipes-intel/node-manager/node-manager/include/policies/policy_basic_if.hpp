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

#include "policy_enums.hpp"
#include "policy_types.hpp"
#include "utility/dbus_enable_if.hpp"

namespace nodemanager
{

/* Forward declaration to break circular dependecy */
class PolicyStateIf;

class PolicyBasicIf : public DbusEnableIf
{
  public:
    PolicyBasicIf(DbusState dbusState) : DbusEnableIf(dbusState)
    {
    }
    virtual ~PolicyBasicIf() = default;

    /**
     * @brief Initialize Policy. Need to be called just after Policy
     * instantation.
     */
    virtual void initialize() = 0;

    /**
     * @brief Get Policy ID
     *
     * @return PolicyId
     */
    virtual PolicyId getId() const = 0;

    /**
     * @brief Get current Policy status
     *
     * @return PolicyState
     */
    virtual PolicyState getState() const = 0;

    /**
     * @brief Install trigger for Policy
     *
     */
    virtual void installTrigger() = 0;

    /**
     * @brief Uninstall trigger for Policy
     *
     */
    virtual void uninstallTrigger() = 0;

    /**
     * @brief Get the Short Object Path object
     *
     * @return const std::string&
     */
    virtual const std::string& getShortObjectPath() const = 0;

    /**
     * @brief Get the Object Path object
     *
     * @return const std::string&
     */
    virtual const std::string& getObjectPath() const = 0;

    /**
     * @brief Get the Owner object
     *
     * @return const PolicyOwner
     */
    virtual PolicyOwner getOwner() const = 0;

    /**
     * @brief Check if policy is editable
     *
     * @return const bool
     */
    virtual bool isEditable() const = 0;

    virtual nlohmann::json toJson() const = 0;
};

} // namespace nodemanager