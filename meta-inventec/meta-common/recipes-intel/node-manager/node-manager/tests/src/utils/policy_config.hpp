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

#include "common_types.hpp"
#include "policies/policy_enums.hpp"
#include "policies/policy_types.hpp"
#include "triggers/trigger_enums.hpp"
#include "utility/enum_to_string.hpp"

#include <map>
#include <variant>
#include <vector>

class PolicyConfig
{
  public:
    using PolicyStorage = nodemanager::PolicyStorage;
    using PowerCorrectionType = nodemanager::PowerCorrectionType;
    using LimitException = nodemanager::LimitException;
    using TriggerType = nodemanager::TriggerType;
    using PolicyParamsTuple = nodemanager::PolicyParamsTuple;
    using PolicyParams = nodemanager::PolicyParams;

    ~PolicyConfig() = default;

    PolicyConfig& correctionInMs(uint32_t val)
    {
        _correctionInMs = val;
        return *this;
    }

    const uint32_t& correctionInMs()
    {
        return _correctionInMs;
    }

    PolicyConfig& limit(uint16_t val)
    {
        _limit = val;
        return *this;
    }

    const uint16_t& limit()
    {
        return _limit;
    }

    PolicyConfig& statReportingPeriod(uint16_t val)
    {
        _statReportingPeriod = val;
        return *this;
    }

    const uint16_t& statReportingPeriod()
    {
        return _statReportingPeriod;
    }

    PolicyConfig& policyStorage(PolicyStorage val)
    {
        _policyStorage = val;
        return *this;
    }

    const PolicyStorage& policyStorage()
    {
        return _policyStorage;
    }

    PolicyConfig& powerCorrectionType(PowerCorrectionType val)
    {
        _powerCorrectionType = val;
        return *this;
    }

    const PowerCorrectionType& powerCorrectionType()
    {
        return _powerCorrectionType;
    }

    PolicyConfig& limitException(LimitException val)
    {
        _limitException = val;
        return *this;
    }

    const LimitException& limitException()
    {
        return _limitException;
    }

    PolicyConfig& suspendPeriods(PolicySuspendPeriods val)
    {
        _suspendPeriods = std::move(val);
        return *this;
    }

    const PolicySuspendPeriods& suspendPeriods()
    {
        return _suspendPeriods;
    }

    PolicyConfig& thresholds(PolicyThresholds val)
    {
        _thresholds = std::move(val);
        return *this;
    }

    const PolicyThresholds& thresholds()
    {
        return _thresholds;
    }

    PolicyConfig& componentId(uint8_t val)
    {
        _componentId = val;
        return *this;
    }

    const uint8_t& componentId()
    {
        return _componentId;
    }

    PolicyConfig& triggerLimit(uint16_t val)
    {
        _triggerLimit = val;
        return *this;
    }

    const uint16_t& triggerLimit()
    {
        return _triggerLimit;
    }

    PolicyConfig& triggerType(TriggerType val)
    {
        _triggerType = val;
        return *this;
    }

    const TriggerType& triggerType()
    {
        return _triggerType;
    }

    PolicyParamsTuple _getTuple()
    {
        return PolicyParamsTuple(
            _correctionInMs, _limit, _statReportingPeriod,
            static_cast<std::underlying_type_t<PolicyStorage>>(_policyStorage),
            static_cast<std::underlying_type_t<PowerCorrectionType>>(
                _powerCorrectionType),
            static_cast<std::underlying_type_t<LimitException>>(
                _limitException),
            _suspendPeriods, _thresholds, _componentId, _triggerLimit,
            enumToStr(kTriggerTypeNames, _triggerType));
    }

    PolicyParams _getStruct()
    {
        PolicyParams params;
        params << _getTuple();
        return params;
    }

  private:
    uint32_t _correctionInMs = 6100;
    uint16_t _limit = 1;
    uint16_t _statReportingPeriod = 10;
    PolicyStorage _policyStorage = PolicyStorage::volatileStorage;
    PowerCorrectionType _powerCorrectionType = PowerCorrectionType::automatic;
    LimitException _limitException = LimitException::noAction;
    PolicySuspendPeriods _suspendPeriods;
    PolicyThresholds _thresholds;
    uint8_t _componentId = 255;
    uint16_t _triggerLimit = 0;
    TriggerType _triggerType = TriggerType::always;
};