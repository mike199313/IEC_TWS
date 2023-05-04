/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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

#include <cmath>

namespace nodemanager
{

template <class T>
struct dependent_false : std::false_type
{
};

/**
 * @brief Returns false if the 'val' goes beyond `lo` and `hi` limits or
 * the T is not an arithmetic type.
 */
template <typename T>
bool isInRange(const T& val, const T& lo, const T& hi)
{
    if constexpr (std::is_arithmetic_v<T>)
    {
        return (val >= lo) && (hi >= val);
    }
    else
    {
        static_assert(dependent_false<T>::value, "Must be arithmetic");
    }
}

/**
 * @brief Throws exception E when val goes beyond `lo` and `hi` limits or
 * the T is not an arithmetic type.
 */
template <class E, class T>
void verifyRange(const T& val, const T& lo, const T& hi)
{
    if (!isInRange(val, lo, hi))
    {
        throw E();
    }
}

/**
 * @brief Returns true if the 'val' conversion to type K is safe.
 */
template <class K, class T>
bool isCastSafe(const T& val)
{
    if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<K>)
    {
        if (std::isnan(val))
        {
            return std::numeric_limits<K>::has_quiet_NaN;
        }
        return (std::numeric_limits<K>::max() >= val) &&
               (std::numeric_limits<K>::lowest() <= val);
    }
    else
    {
        static_assert(dependent_false<T>::value && dependent_false<K>::value,
                      "Must be arithmetic");
    }
}

/**
 * @brief Safely casts to a desired type. In the case of overflow,
 *        returns max value.
 *
 * @param val A value to be casted.
 * @return Returns casted value or max numeric value when not safe
 */
template <class K, class T>
K safeCast(const T& val, const K& returnWhenNotSafe)
{
    if (isCastSafe<K>(val))
    {
        return static_cast<K>(val);
    }
    else
    {
        return returnWhenNotSafe;
    }
}

/**
 * @brief Verifies if it is possible to cast value into T.
 */
template <class T, class K>
static inline bool isEnumCastSafe(const std::set<T>& enumAllValues,
                                  const K& value)
{
    return enumAllValues.count(static_cast<T>(value)) == 1;
}

/**
 * @brief casts value to type T, if cast is not possible then throws E otherwise
 * returns T The enumAllValues set should contain all possible values for enum
 * T.
 */
template <class E, class T, class K>
static inline T toEnum(const std::set<T>& enumAllValues, const K& value)
{
    const auto it = enumAllValues.find(static_cast<T>(value));
    if (it == enumAllValues.cend())
    {
        throw E();
    }
    return *it;
}

} // namespace nodemanager