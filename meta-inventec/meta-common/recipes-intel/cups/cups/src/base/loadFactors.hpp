/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2021 Intel Corporation
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

#include <optional>
#include <string>

namespace cups
{

namespace base
{

enum class LoadFactorCfg
{
    dynamicMode = 0,
    staticMode,
};

struct LoadFactors
{
    static constexpr double defaultLoadFactor = 33.3;
    static constexpr double defaultLoadFactorComplement = 33.4;

    double coreLoadFactor = defaultLoadFactor;
    double iioLoadFactor = defaultLoadFactor;
    double memoryLoadFactor = defaultLoadFactorComplement;

    bool operator!=(const LoadFactors& other) const
    {
        return coreLoadFactor != other.coreLoadFactor ||
               iioLoadFactor != other.iioLoadFactor ||
               memoryLoadFactor != other.memoryLoadFactor;
    }
};

bool validateLoadFactorCfg(const std::string& value);
const std::string& fromLoadFactorCfg(const LoadFactorCfg& loadFactorCfg);
std::optional<LoadFactorCfg>
    toLoadFactorCfg(const std::string& loadFactorCfgStr);
std::tuple<double, double, double> toTuple(const LoadFactors& loadFactors);

} // namespace base

} // namespace cups