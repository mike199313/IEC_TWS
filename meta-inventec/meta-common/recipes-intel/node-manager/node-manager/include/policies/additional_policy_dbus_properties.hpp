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
#include "policy_types.hpp"
#include "utility/dbus_errors.hpp"
#include "utility/property_wrapper.hpp"
#include "utility/ranges.hpp"

#include <memory>

namespace nodemanager
{

class AdditionalPolicyDbusProperties
{
  public:
    AdditionalPolicyDbusProperties()
    {
        setDefaultPropertiesValues();
    }
    AdditionalPolicyDbusProperties(const AdditionalPolicyDbusProperties&) =
        delete;
    AdditionalPolicyDbusProperties&
        operator=(const AdditionalPolicyDbusProperties&) = delete;
    AdditionalPolicyDbusProperties(AdditionalPolicyDbusProperties&&) = delete;
    AdditionalPolicyDbusProperties&
        operator=(AdditionalPolicyDbusProperties&&) = delete;

    virtual ~AdditionalPolicyDbusProperties() = default;

  protected:
    PropertyWrapper<PowerCorrectionType> powerCorrectionType;
    PropertyWrapper<LimitException> limitException;
    PropertyWrapper<uint32_t> correctionTime;

    void registerProperties(
        sdbusplus::asio::dbus_interface& policyAttributesInterface,
        bool editable)
    {
        auto perm = editable == true
                        ? sdbusplus::asio::PropertyPermission::readWrite
                        : sdbusplus::asio::PropertyPermission::readOnly;

        correctionTime.registerProperty(policyAttributesInterface,
                                        "CorrectionInMs", perm);
        powerCorrectionType.registerProperty(policyAttributesInterface,
                                             "PowerCorrectionType", perm);
        limitException.registerProperty(policyAttributesInterface,
                                        "LimitException", perm);
    }

  private:
    void setDefaultPropertiesValues()
    {
        powerCorrectionType.set(PowerCorrectionType::automatic);
        limitException.set(LimitException::noAction);
        correctionTime.set(uint32_t{1});
    }
};

} // namespace nodemanager
