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

#include <future>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <thread>

#include <gmock/gmock.h>

class DbusEnvironment : public ::testing::Environment
{
  public:
    ~DbusEnvironment();

    void SetUp() override;
    void TearDown() override;
    void teardown();

    static boost::asio::io_context& getIoc();
    static std::shared_ptr<sdbusplus::asio::connection> getBus();
    static std::shared_ptr<sdbusplus::asio::object_server> getObjServer();
    static const char* serviceName();
    static std::function<void()> setPromise(std::string_view name);
    static void sleepFor(std::chrono::milliseconds);
    static std::chrono::milliseconds measureTime(std::function<void()>);

    static void synchronizeIoc()
    {
        while (ioc.poll() > 0)
        {
        }
    }

    template <class Functor>
    static void synchronizedPost(Functor&& functor)
    {
        boost::asio::post(ioc, std::forward<Functor>(functor));
        synchronizeIoc();
    }

    template <class T, class F>
    static T waitForFutures(
        std::vector<std::future<T>> futures, T init, F&& accumulator,
        std::chrono::milliseconds timeout = std::chrono::seconds(10))
    {
        constexpr auto precission = std::chrono::milliseconds(10);
        auto elapsed = std::chrono::milliseconds(0);

        auto sum = init;
        for (auto& future : futures)
        {
            while (future.valid() && elapsed < timeout)
            {
                synchronizeIoc();

                if (future.wait_for(precission) == std::future_status::ready)
                {
                    sum = accumulator(sum, future.get());
                }
                else
                {
                    elapsed += precission;
                }
            }
        }

        if (elapsed >= timeout)
        {
            throw std::runtime_error("Timed out while waiting for future");
        }

        return sum;
    }

    template <class T>
    static T waitForFuture(
        std::future<T> future,
        std::chrono::milliseconds timeout = std::chrono::seconds(10))
    {
        std::vector<std::future<T>> futures;
        futures.emplace_back(std::move(future));

        return waitForFutures(
            std::move(futures), T{},
            [](auto, const auto& value) { return value; }, timeout);
    }

    static bool waitForFuture(
        std::string_view name,
        std::chrono::milliseconds timeout = std::chrono::seconds(10));

    static bool waitForFutures(
        std::string_view name,
        std::chrono::milliseconds timeout = std::chrono::seconds(10));

    static bool waitForAllFutures(
        std::chrono::milliseconds timeout = std::chrono::seconds(10));

    static void
        checkForFutures(std::string name, std::function<void()> onIdle,
                        std::chrono::seconds timeout = std::chrono::seconds(5));

    template <typename PropertyType>
    static boost::system::error_code
        setProperty(std::string interface, std::string objectPath,
                    std::string propertyName, PropertyType value)
    {
        auto setPromise = std::promise<boost::system::error_code>();
        auto setFuture = setPromise.get_future();

        sdbusplus::asio::setProperty(*(DbusEnvironment::getBus()),
                                     DbusEnvironment::serviceName(), objectPath,
                                     interface, propertyName, value,
                                     [setPromise = std::move(setPromise)](
                                         boost::system::error_code ec) mutable {
                                         setPromise.set_value(ec);
                                     });

        return DbusEnvironment::waitForFuture(std::move(setFuture));
    }

    template <typename PropertyType>
    static std::tuple<boost::system::error_code, PropertyType>
        getProperty(std::string interface, std::string objectPath,
                    std::string propertyName)
    {
        std::promise<std::tuple<boost::system::error_code, PropertyType>>
            promise;
        sdbusplus::asio::getProperty<PropertyType>(
            *(DbusEnvironment::getBus()), DbusEnvironment::serviceName(),
            objectPath, interface, propertyName,
            [&promise](boost::system::error_code ec, PropertyType data) {
                promise.set_value(std::make_tuple(ec, data));
            });

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

  private:
    static std::future<bool> getFuture(std::string_view name);

    static boost::asio::io_context ioc;
    static std::shared_ptr<sdbusplus::asio::connection> bus;
    static std::shared_ptr<sdbusplus::asio::object_server> objServer;
    static std::map<std::string, std::vector<std::future<bool>>> futures;
    static bool setUp;
};
