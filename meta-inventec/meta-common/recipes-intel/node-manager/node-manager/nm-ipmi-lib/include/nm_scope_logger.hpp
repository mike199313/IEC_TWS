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
#include <nm_logger.hpp>

#define LOG_ENTRY ScopeLogger scopeLogger##__LINE__(__FUNCTION__)

namespace nmipmi
{

class ScopeLogger
{
  public:
    ScopeLogger(const char* msgArg) : msg(msgArg)
    {
        LOGGER_DEBUG << "+ " << msg << "_" << this;
    }

    ~ScopeLogger()
    {
        const auto scope_duration = std::chrono::steady_clock::now() - start;
        LOGGER_DEBUG << "- " << msg << "_" << this << "[duration:"
                     << std::chrono::duration_cast<std::chrono::milliseconds>(
                            scope_duration)
                            .count()
                     << "ms]";
    }

  private:
    const char* msg;
    uint32_t sessionId;
    std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
};

} // namespace nmipmi
