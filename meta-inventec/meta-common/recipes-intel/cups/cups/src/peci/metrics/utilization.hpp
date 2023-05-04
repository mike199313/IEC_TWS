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

#include "log.hpp"
#include "peci/exception.hpp"
#include "traits.hpp"

#include <boost/core/noncopyable.hpp>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

static constexpr double convertToPercent(double val, double max)
{
    if (max == 0)
    {
        // No CPU is available
        return std::numeric_limits<double>::quiet_NaN();
    }

    return (val * 100) / max;
}

template <unsigned BitCount>
class CounterTracker
{
  private:
    constexpr static uint64_t OverflowsAt = utils::bitset_max<BitCount>::value;

    struct Sample
    {
        uint64_t value;
        std::chrono::time_point<std::chrono::steady_clock> time =
            std::chrono::steady_clock::now();
    };

  public:
    std::optional<double> getDelta(const std::string& tag,
                                   const uint64_t sample)
    {
        uint64_t delta;
        Sample current{sample};
        std::optional<double> normalizedDelta;

        if (previous)
        {
            std::chrono::duration<double> period =
                current.time - previous->time;

            if (current.value >= previous->value ||
                OverflowsAt ==
                    std::numeric_limits<decltype(current.value)>().max())
            {
                delta = current.value - previous->value;
            }
            else
            {
                delta = (OverflowsAt - previous->value) + current.value;
            }

            // Normalize delta to diff which would happen in 1s
            normalizedDelta = static_cast<double>(delta) / period.count();

            LOG_DEBUG_T(tag)
                << std::fixed << std::showpoint << std::setprecision(3)
                << "Diff: "
                << "delta(v) = " << delta << " , delta(t) = " << period.count()
                << " , delta(1s) = " << *normalizedDelta;
        }
        previous = current;

        return normalizedDelta;
    }

  private:
    std::optional<Sample> previous;
};

template <class Metric>
class UtilizationDelta : private boost::noncopyable
{
  public:
    UtilizationDelta(Metric& targetArg) : target(targetArg)
    {}

    std::optional<std::pair<double, double>> delta()
    {
        std::optional<std::pair<double, double>> util = std::nullopt;

        std::optional<double> delta = target.delta();
        if (delta)
        {
            util = std::make_pair(*delta, target.getMaxUtil());

            LOG_DEBUG_T(utils::toHex(target.getAddress()))
                << "Utilization = " << to_string(util->first, util->second);
        }

        return util;
    }

    static std::string to_string(double value, double max)
    {
        return std::to_string(static_cast<uint64_t>(value)) + " / " +
               std::to_string(static_cast<uint64_t>(max));
    }

  private:
    Metric target;
};

} // namespace metrics

} // namespace peci

} // namespace cups
