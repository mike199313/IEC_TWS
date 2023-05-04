/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2020 Intel Corporation
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you (License). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or
 *  transmit this software or the related documents without
 *  Intel's prior written permission.
 *
 *  This software and the related documents are provided as is,
 *  with no express or implied warranties, other than those
 *  that are expressly stated in the License.
 */

#pragma once

#include <boost/type_index.hpp>

#include <limits>
#include <type_traits>

namespace cups
{

namespace utils
{

template <unsigned bitCount>
struct bitset_max
{
    static const uint64_t value = (static_cast<uint64_t>(1) << bitCount) - 1;
    static_assert(bitCount < 64, "Values up to 63 are supported.");
};

template <>
struct bitset_max<64>
{
    static const uint64_t value = std::numeric_limits<uint64_t>::max();
};

template <typename T>
static std::string typeName()
{
    return boost::typeindex::type_id<T>().pretty_name();
}

template <typename E>
constexpr auto toIntegral(E e)
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename Array>
constexpr std::size_t std_array_size = std::tuple_size_v<std::decay_t<Array>>;

} // namespace utils

} // namespace cups
