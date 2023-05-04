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

#include "chrono"

namespace nodemanager
{

using MHz = std::chrono::duration<uint32_t, std::mega>;
using HundredMHz =
    std::chrono::duration<uint32_t, std::ratio_multiply<std::mega, std::hecto>>;

class SteadyClockMock
{
  public:
    ~SteadyClockMock() = default;

    typedef std::chrono::nanoseconds duration;
    typedef duration::rep rep;
    typedef duration::period period;
    typedef std::chrono::time_point<std::chrono::steady_clock, duration>
        time_point;

    /**
     * @brief Return the current clock value.
     *
     * @return time_point
     */
    static time_point now() noexcept
    {
        return timePoint;
    }

    /**
     * @brief Increase the current clock value by `delta` miliseconds.
     *
     * @param delta
     */
    static void stepMs(const uint64_t delta) noexcept
    {
        timePoint += std::chrono::milliseconds{delta};
    }

    /**
     * @brief Increase the current clock value by `delta` seconds.
     *
     * @param delta
     */
    static void stepSec(const uint64_t delta) noexcept
    {
        timePoint += std::chrono::seconds{delta};
    }
    static time_point timePoint;
};

SteadyClockMock::time_point SteadyClockMock::timePoint =
    std::chrono::steady_clock::now();

#ifdef CLOCK_MOCK
using Clock = SteadyClockMock;
#else
using Clock = std::chrono::steady_clock;
#endif

} // namespace nodemanager
