/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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

#include "dbus_environment.hpp"

#include <future>
#include <thread>

DbusEnvironment::~DbusEnvironment()
{
    teardown();
}

void DbusEnvironment::SetUp()
{
    if (setUp == false)
    {
        setUp = true;

        bus = std::make_shared<sdbusplus::asio::connection>(ioc);
        bus->request_name(serviceName());

        objServer = std::make_unique<sdbusplus::asio::object_server>(bus);
    }
}

void DbusEnvironment::TearDown()
{
    ioc.poll();

    futures.clear();
}

void DbusEnvironment::teardown()
{
    if (setUp == true)
    {
        setUp = false;

        objServer = nullptr;
        bus = nullptr;
    }
}

boost::asio::io_context& DbusEnvironment::getIoc()
{
    return ioc;
}

std::shared_ptr<sdbusplus::asio::connection> DbusEnvironment::getBus()
{
    return bus;
}

std::shared_ptr<sdbusplus::asio::object_server> DbusEnvironment::getObjServer()
{
    return objServer;
}

const char* DbusEnvironment::serviceName()
{
    return "nm.ut";
}

std::function<void()> DbusEnvironment::setPromise(std::string_view name)
{
    auto promise = std::make_shared<std::promise<bool>>();
    futures[std::string(name)].emplace_back(promise->get_future());
    return [p = std::move(promise)]() { p->set_value(true); };
}

bool DbusEnvironment::waitForFuture(std::string_view name,
                                    std::chrono::milliseconds timeout)
{
    return waitForFuture(getFuture(name), timeout);
}

bool DbusEnvironment::waitForFutures(std::string_view name,
                                     std::chrono::milliseconds timeout)
{
    auto& data = futures[std::string(name)];
    auto ret = waitForFutures(
        std::move(data), true, [](auto sum, auto val) { return sum && val; },
        timeout);
    data = std::vector<std::future<bool>>{};
    return ret;
}

void DbusEnvironment::checkForFutures(std::string name,
                                      std::function<void()> onIdle,
                                      std::chrono::seconds timeout)
{
    constexpr auto precision = std::chrono::milliseconds(10);
    auto elapsed = std::chrono::milliseconds(0);

    auto data = std::move(futures[name]);
    for (auto& future : data)
    {
        while (future.valid() && elapsed < timeout)
        {
            synchronizeIoc();

            if (future.wait_for(precision) == std::future_status::ready)
            {
                future.get();
                break;
            }
            else
            {
                elapsed += precision;
            }

            onIdle();
        }
    }

    if (elapsed >= timeout)
    {
        throw std::runtime_error("Timed out while waiting for future");
    }
}

bool DbusEnvironment::waitForAllFutures(std::chrono::milliseconds timeout)
{
    bool ret = true;
    unsigned count = 0;
    do
    {
        std::vector<std::string> keys;
        count = 0;
        for (const auto& future : futures)
        {
            count += future.second.size();
            if (future.second.size() > 0)
            {
                keys.emplace_back(future.first);
            }
        }
        for (const auto& key : keys)
        {
            ret = ret && waitForFutures(key, timeout);
        }
    } while (count != 0);
    return ret;
}

std::future<bool> DbusEnvironment::getFuture(std::string_view name)
{
    auto& data = futures[std::string(name)];
    auto it = data.begin();

    if (it != data.end())
    {
        auto result = std::move(*it);
        data.erase(it);
        return result;
    }

    return {};
}

void DbusEnvironment::sleepFor(std::chrono::milliseconds timeout)
{
    auto end = std::chrono::high_resolution_clock::now() + timeout;

    while (std::chrono::high_resolution_clock::now() < end)
    {
        synchronizeIoc();
        std::this_thread::yield();
    }

    synchronizeIoc();
}

std::chrono::milliseconds
    DbusEnvironment::measureTime(std::function<void()> fun)
{
    auto begin = std::chrono::high_resolution_clock::now();
    fun();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
}

boost::asio::io_context DbusEnvironment::ioc;
std::shared_ptr<sdbusplus::asio::connection> DbusEnvironment::bus;
std::shared_ptr<sdbusplus::asio::object_server> DbusEnvironment::objServer;
std::map<std::string, std::vector<std::future<bool>>> DbusEnvironment::futures;
bool DbusEnvironment::setUp = false;
