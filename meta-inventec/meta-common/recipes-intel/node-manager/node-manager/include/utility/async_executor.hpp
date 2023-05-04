/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2021 Intel Corporation.
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

#include "boost/asio/steady_timer.hpp"
#include "common_types.hpp"
#include "loggers/log.hpp"
#include "utility/dbus_interfaces.hpp"
#include "utility/property.hpp"
#include "utility/state_if.hpp"

#include <future>
#include <memory>
namespace nodemanager
{

static constexpr const std::chrono::milliseconds kDefaultCheckInterval{20};

template <typename K, typename R>
class AsyncExecutorIf
{
  public:
    using TaskCallback = std::function<void(R)>;
    using Task = std::function<R()>;

    virtual ~AsyncExecutorIf() = default;

    virtual void schedule(const K& key, Task task, TaskCallback callback) = 0;
};

template <class K, class R>
class AsyncExecutor : public AsyncExecutorIf<K, R>
{
  public:
    AsyncExecutor(
        std::shared_ptr<sdbusplus::asio::connection> busArg,
        std::chrono::milliseconds checkIntervalArg = kDefaultCheckInterval) :
        checkResultsTimer(busArg->get_io_context()),
        checkInterval(checkIntervalArg)
    {
        timerHandler();
    };
    virtual ~AsyncExecutor() = default;

    /**
     * @brief Immediately starts asynchronously the `task` if it was not already
     * pending. Does nothing when task or callback are nulls or task with given
     * ‘key’ is still pending.
     *
     * @param key a key that identifies the task
     * @param task Task to execute.  Important Note: the `task` will be executed
     * asynchronously so it must be thread-safe.
     * @param callback a method that will be called when the task is finished.
     * The callback will be called from the thread that is executing current
     * io_context.
     */
    void
        schedule(const K& key, typename AsyncExecutorIf<K, R>::Task task,
                 typename AsyncExecutorIf<K, R>::TaskCallback callback) override
    {
        auto& [f, c] = futureCallbackMap[key];
        if (!f.valid())
        {
            f = std::async(std::launch::async, std::move(task));
            c = std::move(callback);
        }
    }

  private:
    void timerHandler()
    {
        checkResultsTimer.expires_after(checkInterval);
        checkResultsTimer.async_wait([this](boost::system::error_code ec) {
            if (ec)
            {
                Logger::log<LogLevel::info>(
                    "[AsyncExecutor] timer cancelled, ec: %d", ec);
                return;
            }
            executeCallback();
            timerHandler();
        });
    }

    /**
     * @brief Iterates over futureCallbackMap and executes callback for all
     * future objects that has finished, then removes them from the
     * futureCallbackMap.
     *
     */
    void executeCallback()
    {
        for (auto& [key, valuePair] : futureCallbackMap)
        {
            auto& [futureRef, callbackRef] = valuePair;
            if (futureRef.valid() && futureRef.wait_for(std::chrono::seconds(
                                         0)) == std::future_status::ready)
            {
                auto callback = std::move(callbackRef);
                callback(futureRef.get());
            }
        }
    }

    std::map<K, std::tuple<std::future<R>,
                           typename AsyncExecutorIf<K, R>::TaskCallback>>
        futureCallbackMap;
    boost::asio::steady_timer checkResultsTimer;
    std::chrono::milliseconds checkInterval;
};

} // namespace nodemanager
