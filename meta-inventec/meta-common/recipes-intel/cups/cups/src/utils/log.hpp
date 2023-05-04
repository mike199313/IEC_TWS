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

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace cups
{

namespace utils
{

enum class LogLevel
{
    most_verbose = 0,
    debug = most_verbose,
    info,
    warning,
    error,
    critical,
    least_verbose = critical
};

class logger
{
  private:
    static std::string timestamp()
    {
        std::string date;
        date.resize(32, '\0');
        time_t t = time(nullptr);

        tm myTm{};

        gmtime_r(&t, &myTm);

        size_t sz =
            strftime(date.data(), date.size(), "%Y-%m-%d %H:%M:%S", &myTm);
        date.resize(sz);
        return date;
    }

  public:
    logger(const std::string& prefix, const std::string& file,
           const size_t line, LogLevel levelIn, const std::string& tag = {}) :
        level(levelIn)
    {
#ifdef ENABLE_LOGS
        stringstream << "(" << timestamp() << ") ";
        stringstream << "[" << prefix << "] ";
        stringstream << "{" << std::filesystem::path(file).filename().string()
                     << ":" << line << "} ";
#endif
    }

    ~logger()
    {
        if (level >= get_current_log_level())
        {
#ifdef ENABLE_LOGS
            stringstream << std::endl;
            std::cerr << stringstream.str();
#endif
        }
    }

    //
    template <typename T>
    logger& operator<<(T const& value)
    {
        if (level >= get_current_log_level())
        {
#ifdef ENABLE_LOGS
            stringstream << value;
#endif
        }
        return *this;
    }

    static void setLogLevel(LogLevel level)
    {
        getLogLevelRef() = level;
    }

    static LogLevel get_current_log_level()
    {
        return getLogLevelRef();
    }

  private:
    static LogLevel& getLogLevelRef()
    {
        static auto currentLevel = static_cast<LogLevel>(LogLevel::debug);
        return currentLevel;
    }

    std::ostringstream stringstream;
    LogLevel level;
};

template <typename T>
static inline std::string toHex(const T& value)
{
    std::stringstream ss;
    ss << "0x" << std::setw(sizeof(T) * 2) << std::setfill('0') << std::hex
       << static_cast<int>(value);
    return ss.str();
}

} // namespace utils

} // namespace cups

#define _LOG_COMMON(_level_, _level_name_)                                     \
    if (cups::utils::logger::get_current_log_level() <= (_level_))             \
    cups::utils::logger((_level_name_), __FILE__, __LINE__, (_level_))

#define LOG_CRITICAL _LOG_COMMON(cups::utils::LogLevel::critical, "CRITICAL")
#define LOG_ERROR _LOG_COMMON(cups::utils::LogLevel::error, "ERROR")
#define LOG_WARNING _LOG_COMMON(cups::utils::LogLevel::warning, "WARNING")
#define LOG_INFO _LOG_COMMON(cups::utils::LogLevel::info, "INFO")
#define LOG_DEBUG _LOG_COMMON(cups::utils::LogLevel::debug, "DEBUG")

#define LOG_CRITICAL_T(_tag_) LOG_CRITICAL << "[" << (_tag_) << "] "
#define LOG_ERROR_T(_tag_) LOG_ERROR << "[" << (_tag_) << "] "
#define LOG_WARNING_T(_tag_) LOG_WARNING << "[" << (_tag_) << "] "
#define LOG_INFO_T(_tag_) LOG_INFO << "[" << (_tag_) << "] "
#define LOG_DEBUG_T(_tag_) LOG_DEBUG << "[" << (_tag_) << "] "
