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

#include <string>
#include <unordered_map>

namespace nodemanager
{

/**
 * @brief Get provided enum value name
 *
 * @tparam E - enum type
 * @param names - list of possible names
 * @param value - enum value
 * @return std::string - enum value name
 */
template <class E>
std::string enumToStr(std::unordered_map<E, std::string> names, E value)
{
    auto name = names.find(value);
    if (name == names.end())
    {
        return std::to_string(std::underlying_type_t<E>(value));
    }
    return name->second;
}

template <class E, class T>
inline T strToEnum(const std::unordered_map<T, std::string>& data,
                   const std::string& value)
{
    auto it =
        std::find_if(data.begin(), data.end(), [&value](const auto& item) {
            return item.second == value;
        });
    if (it == data.end())
    {
        throw E();
    }
    return it->first;
}

} // namespace nodemanager