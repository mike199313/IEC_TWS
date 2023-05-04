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

#include "dbus_errors.hpp"
#include "policies/policy_enums.hpp"
#include "triggers/trigger_enums.hpp"
#include "utility/enum_to_string.hpp"
#include "utility/tag.hpp"

namespace nodemanager
{
namespace utility
{
template <class T>
struct simple_type : public std::underlying_type<T>,
                     public std::enable_if<!std::is_enum_v<T>, T>
{
};
template <class T>
using simple_type_t = typename simple_type<T>::type;

template <class... Args>
struct Types
{
  public:
    template <class F>
    static void for_each(F&& functor)
    {
        (functor(nodemanager::Tag<Args>()), ...);
    }
};

template <typename T>
struct DbusPropertyTraits
{
    using DbusType = simple_type_t<T>;

    static T fromDbusType(DbusType value)
    {
        return static_cast<T>(value);
    }

    static DbusType toDbusType(T value)
    {
        return static_cast<DbusType>(value);
    }
};

template <>
struct DbusPropertyTraits<TriggerType>
{
    using DbusType = std::string;

    static TriggerType fromDbusType(DbusType str)
    {
        return strToEnum<errors::UnsupportedPolicyTriggerType>(
            kTriggerTypeNames, str);
    }

    static DbusType toDbusType(TriggerType element)
    {
        return enumToStr(kTriggerTypeNames, element);
    }
};

} // namespace utility
} // namespace nodemanager
