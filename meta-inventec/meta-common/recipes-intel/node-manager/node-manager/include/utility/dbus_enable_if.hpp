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

#include "common_types.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/dbus_interfaces.hpp"
#include "utility/property.hpp"
#include "utility/state_if.hpp"

#include <memory>

namespace nodemanager
{

static constexpr bool kDbusObjectEnabledByDefault = true;
static constexpr bool kDbusObjectDisabledByDefault = false;

using DbusEnableChangeCallback = std::function<void(void)>;

enum class DbusState
{
    disabled,
    enabled,
    alwaysEnabled,
};

class DbusEnableIf : public StateIf
{
  public:
    DbusEnableIf(DbusState stateArg = DbusState::enabled)
    {
        isLoadedFlag = ((stateArg == DbusState::enabled) ||
                        (stateArg == DbusState::alwaysEnabled));
        changeAllowed = (stateArg != DbusState::alwaysEnabled);
    }
    virtual ~DbusEnableIf() = default;

    virtual void onStateChanged() = 0;

    /**
     * @brief Returns true if this object and its parent are Enabled.
     */
    virtual bool isRunning() const
    {
        return isLoadedFlag && isParentRunning;
    }

    virtual bool isParentEnabled() const
    {
        return isParentRunning;
    }

    virtual void setParentRunning(const bool value)
    {
        bool currentRunningState = isRunning();
        isParentRunning = value;
        if (currentRunningState != isRunning())
        {
            onStateChanged();
        }
    }

    virtual bool isEnabledOnDbus() const
    {
        return isLoadedFlag;
    }

  protected:
    void initializeDbusInterfaces(
        DbusInterfaces& dbusInterfaces,
        const DbusEnableChangeCallback& callback = nullptr)
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.Object.Enable",
            [this, &callback](auto& iface) {
                iface.register_property(
                    "Enabled", isLoadedFlag,
                    [this, callback](const auto& newValue,
                                     auto& propertyValue) {
                        if (!changeAllowed)
                        {
                            throw errors::OperationNotPermitted();
                        }
                        if (newValue != propertyValue)
                        {
                            propertyValue = newValue;
                            isLoadedFlag = newValue;
                            onStateChanged();

                            if (callback)
                            {
                                callback();
                            }
                            return true;
                        }
                        return false;
                    },
                    [this](const auto& property) { return isLoadedFlag; });
            });
    }

  private:
    bool isParentRunning = true;
    bool isLoadedFlag = true;
    bool changeAllowed = true;
};

} // namespace nodemanager
