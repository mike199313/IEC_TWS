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

#include "log_level.hpp"

#include <boost/format.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>

namespace nodemanager
{

class DebugLogger
{
  public:
    template <LogLevel logLevel>
    static void log(const std::string& msg)
    {
        if (logLevel >= currentLogLevel)
        {
            logWrapperCache<logLevel>(msg.c_str());
        }
    }

    template <LogLevel logLevel>
    static void log(const char* msg)
    {
        if (logLevel >= currentLogLevel)
        {
            logWrapperCache<logLevel>(msg);
        }
    }

    template <LogLevel logLevel, typename... Args>
    static void log(const char* msg, Args&&... args)
    {
        if (logLevel >= currentLogLevel)
        {
            logWrapperCache<logLevel>(
                format(msg, std::forward<Args>(args)...).c_str());
        }
    }

    static LogLevel getLogLevel()
    {
        return currentLogLevel;
    }

    static void setLogLevel(LogLevel newValue)
    {
        currentLogLevel = newValue;
    }

    static uint16_t getRateLimitBurst()
    {
        return rateLimitBurst;
    }

    static void setRateLimitBurst(uint16_t newValue)
    {
        rateLimitBurst = newValue;
    }

    static int64_t getRateLimitInterval()
    {
        return rateLimitInterval.count();
    }

    static void setRateLimitInterval(int64_t newValue)
    {
        rateLimitInterval = std::chrono::seconds(newValue);
    }

  private:
    static LogLevel currentLogLevel;
    static uint16_t rateLimitBurst;
    static std::chrono::seconds rateLimitInterval;

    template <LogLevel logLevel>
    static void logWrapper(const char* msg)
    {
        phosphor::logging::log<journalLevel<logLevel>()>(msg);
    }

    template <LogLevel logLevel>
    static void logWrapperCache(const char* msg)
    {
        if (!isRateLimitEnabled())
        {
            logWrapper<logLevel>(msg);
            return;
        }

        msgCounter++;
        if (msgCounter < rateLimitBurst)
        {
            logWrapper<logLevel>(msg);
        }
        else if (msgCounter == rateLimitBurst)
        {
            logWrapper<logLevel>(msg);
            const auto remainingTimeWindow =
                (rateLimitInterval - elapsedTimeUntilNow(prevTimestamp))
                    .count();
            if (remainingTimeWindow <= 0)
            {
                msgCounter = 0;
                prevTimestamp = Clock::now();
            }
            else
            {
                std::string output =
                    format("Log RateLimitBurst reached, supprssing next "
                           "messages through: %1% sec",
                           remainingTimeWindow);
                logWrapper<LogLevel::warning>(output.c_str());
            }
        }
        else
        {
            if (elapsedTimeUntilNow(prevTimestamp) > rateLimitInterval)
            {
                msgCounter = 0;
                prevTimestamp = Clock::now();
            }
        }
    }

    template <typename... Args>
    static std::string format(const char* msg, Args&&... args)
    {
        std::stringstream ss;
        ss << (boost::format(msg) % ... % args);
        return ss.str();
    }

    static inline bool isRateLimitEnabled()
    {
        if (LogLevel::debug == currentLogLevel)
        {
            return false;
        }
        return rateLimitBurst != 0 && rateLimitInterval.count() != 0;
    }

    static inline std::chrono::seconds
        elapsedTimeUntilNow(Clock::time_point referenceTimePoint)
    {
        const auto now = Clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            now - referenceTimePoint);
    }

    static Clock::time_point prevTimestamp;
    static uint16_t msgCounter;
};

Clock::time_point DebugLogger::prevTimestamp = Clock::now();
uint16_t DebugLogger::msgCounter = 0;
LogLevel DebugLogger::currentLogLevel{LogLevel::info};
uint16_t DebugLogger::rateLimitBurst{300};
std::chrono::seconds DebugLogger::rateLimitInterval{1 * 60};

} // namespace nodemanager
