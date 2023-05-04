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

#include <phosphor-logging/log.hpp>
namespace nodemanager
{

enum class LogLevel
{
    debug = 0,
    info,
    warning,
    error,
    critical,
};

static const std::unordered_map<LogLevel, std::string> kLogLevelNames = {
    {LogLevel::debug, "debug"},
    {LogLevel::info, "info"},
    {LogLevel::warning, "warning"},
    {LogLevel::error, "error"},
    {LogLevel::critical, "critical"}};

template <LogLevel logLevel>
constexpr phosphor::logging::level journalLevel()
{
    if constexpr (logLevel == LogLevel::debug)
    {
        // the OpenBMC journal doesn't store/print DEBUG logs by default, so
        // this INFO level here is to make NM debug level logs visible in
        // journal.
        return phosphor::logging::level::INFO;
    }
    else if constexpr (logLevel == LogLevel::info)
    {
        return phosphor::logging::level::INFO;
    }
    else if constexpr (logLevel == LogLevel::warning)
    {
        return phosphor::logging::level::WARNING;
    }
    else if constexpr (logLevel == LogLevel::error)
    {
        return phosphor::logging::level::ERR;
    }
    else
    {
        return phosphor::logging::level::CRIT;
    }
}

} // namespace nodemanager
