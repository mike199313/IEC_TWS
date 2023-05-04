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

#include "loggers/log.hpp"
#include "policy_basic_if.hpp"
#include "policy_state_if.hpp"
#include "utility/enum_to_string.hpp"

namespace nodemanager
{

/**
 * @brief Mapping Policy state with its name
 */
std::unordered_map<PolicyState, std::string> policyStateNames = {
    {PolicyState::disabled, "DISABLED"}, {PolicyState::ready, "READY"},
    {PolicyState::pending, "PENDING"},   {PolicyState::triggered, "TRIGGERED"},
    {PolicyState::selected, "SELECTED"}, {PolicyState::suspended, "SUSPENDED"}};

class PolicyStateBase : public PolicyStateIf
{
  protected:
    PolicyStateBase() = default;

  public:
    PolicyStateBase& operator=(const PolicyStateBase&) = delete;
    PolicyStateBase& operator=(PolicyStateBase&&) = delete;
    virtual ~PolicyStateBase() = default;
    virtual void initialize(
        std::shared_ptr<PolicyIf> policyArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg) override;
    virtual std::unique_ptr<PolicyStateIf>
        onParametersValidation(bool isValid) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onEnabled(bool isPolicyEnabled) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParentEnabled(bool isParentEnabled) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onTriggerAction(TriggerActionType at) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onLimitSelection(bool isLimitSelected) const override;

  protected:
    inline std::unique_ptr<PolicyStateIf> doNotChangeState() const;
    template <class T>
    constexpr inline std::unique_ptr<PolicyStateIf> switchToState(bool v) const
    {
        if (v)
        {
            return std::make_unique<T>();
        }
        return doNotChangeState();
    }
    std::weak_ptr<PolicyIf> wpPolicy;
    std::shared_ptr<sdbusplus::asio::connection> bus;
};

class PolicyStateSuspended : public PolicyStateBase
{
  public:
    PolicyStateSuspended& operator=(const PolicyStateSuspended&) = delete;
    PolicyStateSuspended& operator=(PolicyStateSuspended&&) = delete;
    PolicyStateSuspended() = default;
    virtual ~PolicyStateSuspended();
    virtual void initialize(
        std::shared_ptr<PolicyIf> policyArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg) override;
    virtual inline PolicyState getState() const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParametersValidation(bool isValid) const override;
};

class PolicyStateDisabled : public PolicyStateBase
{
  public:
    PolicyStateDisabled& operator=(const PolicyStateDisabled&) = delete;
    PolicyStateDisabled& operator=(PolicyStateDisabled&&) = delete;
    PolicyStateDisabled() = default;
    virtual ~PolicyStateDisabled();
    virtual void initialize(
        std::shared_ptr<PolicyIf> policyArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg) override;
    virtual inline PolicyState getState() const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParametersValidation(bool isValid) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onEnabled(bool isPolicyEnabled) const override;
};

class PolicyStatePending : public PolicyStateBase
{
  public:
    PolicyStatePending& operator=(const PolicyStatePending&) = delete;
    PolicyStatePending& operator=(PolicyStatePending&&) = delete;
    PolicyStatePending() = default;
    virtual ~PolicyStatePending() = default;
    virtual void initialize(
        std::shared_ptr<PolicyIf> policyArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg) override;
    virtual inline PolicyState getState() const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParentEnabled(bool isParentEnabled) const override;
};

class PolicyStateReady : public PolicyStateBase
{
  public:
    PolicyStateReady& operator=(const PolicyStateReady&) = delete;
    PolicyStateReady& operator=(PolicyStateReady&&) = delete;
    PolicyStateReady() = default;
    virtual ~PolicyStateReady() = default;
    virtual void initialize(
        std::shared_ptr<PolicyIf> policyArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg) override;
    virtual inline PolicyState getState() const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParentEnabled(bool isParentEnabled) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onTriggerAction(TriggerActionType at) const override;
};

class PolicyStateTriggered : public PolicyStateBase
{
  public:
    PolicyStateTriggered& operator=(const PolicyStateTriggered&) = delete;
    PolicyStateTriggered& operator=(PolicyStateTriggered&&) = delete;
    PolicyStateTriggered() = default;
    virtual ~PolicyStateTriggered() = default;
    virtual inline PolicyState getState() const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParentEnabled(bool isParentEnabled) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onTriggerAction(TriggerActionType at) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onLimitSelection(bool isLimitSelected) const override;
};

class PolicyStateSelected : public PolicyStateBase
{
  public:
    PolicyStateSelected& operator=(const PolicyStateSelected&) = delete;
    PolicyStateSelected& operator=(PolicyStateSelected&&) = delete;
    PolicyStateSelected() = default;
    virtual ~PolicyStateSelected();
    virtual void initialize(
        std::shared_ptr<PolicyIf> policyArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg) override;
    virtual inline PolicyState getState() const override;
    virtual std::unique_ptr<PolicyStateIf>
        onParentEnabled(bool isParentEnabled) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onTriggerAction(TriggerActionType at) const override;
    virtual std::unique_ptr<PolicyStateIf>
        onLimitSelection(bool isLimitSelected) const override;

  private:
    std::function<void(void)> logStopCallback;
};

//-------------------------PolicyStateBase--------------------------------------
void PolicyStateBase::initialize(
    std::shared_ptr<PolicyIf> policyArg,
    std::shared_ptr<sdbusplus::asio::connection> busArg)
{
    wpPolicy = policyArg;
    bus = std::move(busArg);
    Logger::log<LogLevel::info>("Policy %s enters state %s\n",
                                policyArg->getShortObjectPath(),
                                enumToStr(policyStateNames, getState()));
}

std::unique_ptr<PolicyStateIf>
    PolicyStateBase::onParametersValidation(bool isValid) const
{
    return switchToState<PolicyStateSuspended>(!isValid);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateBase::onEnabled(bool isPolicyEnabled) const
{
    return switchToState<PolicyStateDisabled>(!isPolicyEnabled);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateBase::onParentEnabled(bool isParentEnabled) const
{
    return doNotChangeState();
}

std::unique_ptr<PolicyStateIf>
    PolicyStateBase::onTriggerAction(TriggerActionType at) const
{
    return doNotChangeState();
}

std::unique_ptr<PolicyStateIf>
    PolicyStateBase::onLimitSelection(bool isLimitSelected) const
{
    return doNotChangeState();
}

inline std::unique_ptr<PolicyStateIf> PolicyStateBase::doNotChangeState() const
{
    return std::unique_ptr<PolicyStateIf>(nullptr);
}

//-------------------------PolicyStateSuspended---------------------------------
PolicyStateSuspended::~PolicyStateSuspended()
{
    if (bus)
    {
        bus->get_io_context().post([wp = wpPolicy]() {
            if (auto p = wp.lock())
            {
                if (p->getState() == PolicyState::pending)
                {
                    p->onStateChanged();
                }
            }
        });
    }
}

void PolicyStateSuspended::initialize(
    std::shared_ptr<PolicyIf> policyArg,
    std::shared_ptr<sdbusplus::asio::connection> busArg)
{
    PolicyStateBase::initialize(policyArg, busArg);
    if (auto policy = wpPolicy.lock())
    {
        policy->uninstallTrigger();
    }
}

inline PolicyState PolicyStateSuspended::getState() const
{
    return PolicyState::suspended;
}

std::unique_ptr<PolicyStateIf>
    PolicyStateSuspended::onParametersValidation(bool isValid) const
{
    return switchToState<PolicyStatePending>(isValid);
}

//-------------------------PolicyStateDisabled----------------------------------
PolicyStateDisabled::~PolicyStateDisabled()
{
    if (bus)
    {
        bus->get_io_context().post([wp = wpPolicy]() {
            if (auto p = wp.lock())
            {
                p->validateParameters();
                p->onStateChanged();
            }
        });
    }
}

void PolicyStateDisabled::initialize(
    std::shared_ptr<PolicyIf> policyArg,
    std::shared_ptr<sdbusplus::asio::connection> busArg)
{
    PolicyStateBase::initialize(policyArg, busArg);
    if (auto policy = wpPolicy.lock())
    {
        policy->uninstallTrigger();
    }
}

inline PolicyState PolicyStateDisabled::getState() const
{
    return PolicyState::disabled;
}

std::unique_ptr<PolicyStateIf>
    PolicyStateDisabled::onParametersValidation(bool isValid) const
{
    return doNotChangeState();
}

std::unique_ptr<PolicyStateIf>
    PolicyStateDisabled::onEnabled(bool isPolicyEnabled) const
{
    return switchToState<PolicyStatePending>(isPolicyEnabled);
}

//-------------------------PolicyStatePending-----------------------------------

void PolicyStatePending::initialize(
    std::shared_ptr<PolicyIf> policyArg,
    std::shared_ptr<sdbusplus::asio::connection> busArg)
{
    PolicyStateBase::initialize(policyArg, busArg);
    if (auto policy = wpPolicy.lock())
    {
        policy->uninstallTrigger();
    }
}

inline PolicyState PolicyStatePending::getState() const
{
    return PolicyState::pending;
}

std::unique_ptr<PolicyStateIf>
    PolicyStatePending::onParentEnabled(bool isParentEnabled) const
{
    return switchToState<PolicyStateReady>(isParentEnabled);
}

//-------------------------PolicyStateReady-------------------------------------
void PolicyStateReady::initialize(
    std::shared_ptr<PolicyIf> policyArg,
    std::shared_ptr<sdbusplus::asio::connection> busArg)
{
    PolicyStateBase::initialize(policyArg, busArg);
    if (auto policy = wpPolicy.lock())
    {
        policy->installTrigger();
    }
}

inline PolicyState PolicyStateReady::getState() const
{
    return PolicyState::ready;
}

std::unique_ptr<PolicyStateIf>
    PolicyStateReady::onParentEnabled(bool isParentEnabled) const
{
    return switchToState<PolicyStatePending>(!isParentEnabled);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateReady::onTriggerAction(TriggerActionType at) const
{
    return switchToState<PolicyStateTriggered>(at ==
                                               TriggerActionType::trigger);
}

//-------------------------PolicyStateTriggered---------------------------------
inline PolicyState PolicyStateTriggered::getState() const
{
    return PolicyState::triggered;
}

std::unique_ptr<PolicyStateIf>
    PolicyStateTriggered::onParentEnabled(bool isParentEnabled) const
{
    return switchToState<PolicyStatePending>(!isParentEnabled);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateTriggered::onLimitSelection(bool isLimitSelected) const
{
    return switchToState<PolicyStateSelected>(isLimitSelected);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateTriggered::onTriggerAction(TriggerActionType at) const
{
    return switchToState<PolicyStateReady>(at == TriggerActionType::deactivate);
}

//-------------------------PolicyStateSelected----------------------------------
PolicyStateSelected::~PolicyStateSelected()
{
    if (logStopCallback)
    {
        logStopCallback();
    }
}

void PolicyStateSelected::initialize(
    std::shared_ptr<PolicyIf> policyArg,
    std::shared_ptr<sdbusplus::asio::connection> busArg)
{
    PolicyStateBase::initialize(policyArg, busArg);
    if (auto policy = wpPolicy.lock())
    {
        logStopCallback = ThrottlingLogger::logStart(policy);
    }
}

inline PolicyState PolicyStateSelected::getState() const
{
    return PolicyState::selected;
}

std::unique_ptr<PolicyStateIf>
    PolicyStateSelected::onParentEnabled(bool isParentEnabled) const
{
    return switchToState<PolicyStatePending>(!isParentEnabled);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateSelected::onTriggerAction(TriggerActionType at) const
{
    return switchToState<PolicyStateReady>(at == TriggerActionType::deactivate);
}

std::unique_ptr<PolicyStateIf>
    PolicyStateSelected::onLimitSelection(bool isLimitSelected) const
{
    return switchToState<PolicyStateTriggered>(!isLimitSelected);
}
} // namespace nodemanager
