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

#include "base/loadFactors.hpp"
#include "utils/log.hpp"

#include <boost/container/flat_map.hpp>

#include <algorithm>
#include <stdexcept>

namespace cups
{

namespace base
{

static const boost::container::flat_map<std::string, LoadFactorCfg>
    loadFactorCfgMap = {{"Dynamic", LoadFactorCfg::dynamicMode},
                        {"Static", LoadFactorCfg::staticMode}};

bool validateLoadFactorCfg(const std::string& value)
{
    if (loadFactorCfgMap.contains(value))
    {
        return true;
    }

    return false;
}

const std::string& fromLoadFactorCfg(const LoadFactorCfg& loadFactorCfg)
{
    // enum to string
    auto it = std::find_if(loadFactorCfgMap.begin(), loadFactorCfgMap.end(),
                           [loadFactorCfg](const auto& mapItem) {
                               return mapItem.second == loadFactorCfg;
                           });
    if (it != loadFactorCfgMap.end())
    {
        return it->first;
    }

    throw std::runtime_error("Invalid load factor configuration!");
}

std::optional<LoadFactorCfg>
    toLoadFactorCfg(const std::string& loadFactorCfgStr)
{
    // string to enum
    auto it = loadFactorCfgMap.find(loadFactorCfgStr);
    if (it != loadFactorCfgMap.end())
    {
        return it->second;
    }

    LOG_ERROR << "Invalid load factor configuration: " << loadFactorCfgStr;
    return std::nullopt;
}

std::tuple<double, double, double> toTuple(const LoadFactors& loadFactors)
{
    return {loadFactors.coreLoadFactor, loadFactors.iioLoadFactor,
            loadFactors.memoryLoadFactor};
}
} // namespace base

} // namespace cups