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

#include <sdbusplus/asio/object_server.hpp>

namespace nodemanager
{

template <class T, class Cleaner>
class ScopedResource
{
  public:
    ScopedResource() = default;
    ScopedResource(T resourceArg, Cleaner cleanerArg) :
        resource(std::move(resourceArg)), cleaner(std::move(cleanerArg))
    {
    }
    ScopedResource(const ScopedResource&) = delete;
    ScopedResource(ScopedResource&& other) :
        resource(std::move(other.resource)), cleaner(std::move(other.cleaner))
    {
        other.resource = std::nullopt;
        other.cleaner = std::nullopt;
    }

    ScopedResource& operator=(const ScopedResource&) = delete;
    ScopedResource& operator=(ScopedResource&& other)
    {
        clear();

        std::swap(cleaner, other.cleaner);
        std::swap(resource, other.resource);

        return *this;
    }

    ~ScopedResource()
    {
        clear();
    }

    T& operator*()
    {
        return resource.value();
    }

    const T& operator*() const
    {
        return resource.value();
    }

    T* operator->()
    {
        return &resource.value();
    }

    const T* operator->() const
    {
        return &resource.value();
    }

  private:
    void clear()
    {
        if (cleaner and resource)
        {
            (*cleaner)(*resource);
        }

        cleaner = std::nullopt;
        resource = std::nullopt;
    }

    std::optional<T> resource;
    std::optional<Cleaner> cleaner;
};

using DbusScopedInterface = ScopedResource<
    std::shared_ptr<sdbusplus::asio::dbus_interface>,
    std::function<void(std::shared_ptr<sdbusplus::asio::dbus_interface>&)>>;

auto makeDbusInterfaceRemover(
    std::shared_ptr<sdbusplus::asio::object_server> server)
{
    return [server](std::shared_ptr<sdbusplus::asio::dbus_interface>& ifce) {
        return server->remove_interface(ifce);
    };
}

} // namespace nodemanager
