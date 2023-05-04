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
#include <sstream>

namespace nmipmi
{

#ifdef ENABLE_LOGGING
static constexpr const auto kCurrentLogLevel = phosphor::logging::level::DEBUG;
#else
static constexpr const auto kCurrentLogLevel = phosphor::logging::level::EMERG;
#endif

template <phosphor::logging::level L>
class StreamLogger
{
  public:
    template <typename T>
    StreamLogger& operator<<([[maybe_unused]] T const& value)
    {
        if constexpr (L <= kCurrentLogLevel)
        {
            stringstream << value;
        }
        return *this;
    }

    ~StreamLogger()
    {
        if constexpr (L <= kCurrentLogLevel)
        {
            stringstream << std::endl;
            phosphor::logging::log<L>(stringstream.str().c_str());
        }
    }

  private:
    std::ostringstream stringstream;
};

} // namespace nmipmi

#define LOGGER_CRIT                                                            \
    if constexpr (phosphor::logging::level::CRIT <= kCurrentLogLevel)          \
    StreamLogger<phosphor::logging::level::CRIT>()
#define LOGGER_ERR                                                             \
    if constexpr (phosphor::logging::level::ERR <= kCurrentLogLevel)           \
    StreamLogger<phosphor::logging::level::ERR>()
#define LOGGER_WARN                                                            \
    if constexpr (phosphor::logging::level::WARNING <= kCurrentLogLevel)       \
    StreamLogger<phosphor::logging::level::WARNING>()
#define LOGGER_INFO                                                            \
    if constexpr (phosphor::logging::level::INFO <= kCurrentLogLevel)          \
    StreamLogger<phosphor::logging::level::INFO>()
#define LOGGER_DEBUG                                                           \
    if constexpr (phosphor::logging::level::DEBUG <= kCurrentLogLevel)         \
    StreamLogger<phosphor::logging::level::DEBUG>()
