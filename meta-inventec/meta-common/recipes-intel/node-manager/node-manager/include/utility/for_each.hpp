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

#include <tuple>
#include <utility>

namespace nodemanager
{

template <class... Args, class F, size_t... Seq>
void for_each(const std::tuple<Args...>& tuple, F&& fun,
              std::index_sequence<Seq...>)
{
    ((std::apply(fun, std::get<Seq>(tuple))), ...);
}

template <class... Args, class F>
void for_each(const std::tuple<Args...>& tuple, F&& fun)
{
    const auto seq = std::make_index_sequence<sizeof...(Args)>();
    for_each(tuple, std::forward<F>(fun), seq);
}

} // namespace nodemanager
