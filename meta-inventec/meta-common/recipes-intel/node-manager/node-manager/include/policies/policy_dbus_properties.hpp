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
#include "policy_types.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/property_wrapper.hpp"
#include "utility/ranges.hpp"

#include <memory>

namespace nodemanager
{

class PolicyDbusProperties
{
  public:
    PolicyDbusProperties(PolicyId& id) : policyId(id)
    {
        setDefaultPropertiesValues();
    }
    PolicyDbusProperties(const PolicyDbusProperties&) = delete;
    PolicyDbusProperties& operator=(const PolicyDbusProperties&) = delete;
    PolicyDbusProperties(PolicyDbusProperties&&) = delete;
    PolicyDbusProperties& operator=(PolicyDbusProperties&&) = delete;

    virtual ~PolicyDbusProperties() = default;

  protected:
    ConstProperty<PolicyId> policyId;
    PropertyWrapper<std::string> domainId;
    PropertyWrapper<PolicyOwner> owner;
    PropertyWrapper<uint16_t> limit;
    PropertyWrapper<PolicyStorage> policyStorage;
    PropertyWrapper<DeviceIndex> componentId;
    PropertyWrapper<uint16_t> statReportingPeriod;
    PropertyWrapper<TriggerType> triggerType;
    PropertyWrapper<uint16_t> triggerLimit;
    PropertyWrapper<PolicyState> policyState;
    PropertyWrapper<std::string> policyType;

    void registerProperties(
        sdbusplus::asio::dbus_interface& policyAttributesInterface,
        bool editable)
    {
        auto perm = editable == true
                        ? sdbusplus::asio::PropertyPermission::readWrite
                        : sdbusplus::asio::PropertyPermission::readOnly;

        policyId.registerProperty(policyAttributesInterface, "Id");
        owner.registerProperty(policyAttributesInterface, "Owner",
                               sdbusplus::asio::PropertyPermission::readOnly);
        limit.registerProperty(policyAttributesInterface, "Limit", perm);
        policyStorage.registerProperty(policyAttributesInterface,
                                       "PolicyStorage", perm);
        domainId.registerProperty(
            policyAttributesInterface, "DomainId",
            sdbusplus::asio::PropertyPermission::readOnly);
        componentId.registerProperty(
            policyAttributesInterface, "ComponentId",
            sdbusplus::asio::PropertyPermission::readOnly);
        statReportingPeriod.registerProperty(
            policyAttributesInterface, "StatisticsReportingPeriod",
            sdbusplus::asio::PropertyPermission::readOnly);
        triggerType.registerProperty(
            policyAttributesInterface, "TriggerType",
            sdbusplus::asio::PropertyPermission::readOnly);
        triggerLimit.registerProperty(
            policyAttributesInterface, "TriggerLimit",
            sdbusplus::asio::PropertyPermission::readOnly);
        policyState.registerProperty(
            policyAttributesInterface, "PolicyState",
            sdbusplus::asio::PropertyPermission::readOnly);
        policyType.registerProperty(
            policyAttributesInterface, "PolicyType",
            sdbusplus::asio::PropertyPermission::readOnly);
    }

  private:
    void setDefaultPropertiesValues()
    {
        limit.set(uint16_t{0});
        policyStorage.set(PolicyStorage::volatileStorage);
        componentId.set(kComponentIdAll);
        statReportingPeriod.set(uint16_t{0});
        triggerLimit.set(uint16_t{0});
        triggerType.set(TriggerType::always);
        policyState.set(PolicyState::disabled);
    }
};

} // namespace nodemanager
