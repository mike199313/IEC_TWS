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

#include "common_types.hpp"
#include "scoped_resource.hpp"
#include "utility/property.hpp"

namespace nodemanager
{

class DbusInterfaces
{
  public:
    DbusInterfaces(const DbusInterfaces&) = delete;
    DbusInterfaces(DbusInterfaces&&) = delete;
    DbusInterfaces& operator=(const DbusInterfaces&) = delete;
    DbusInterfaces& operator=(DbusInterfaces&&) = delete;

    DbusInterfaces(const std::string& objectPathArg,
                   const std::shared_ptr<sdbusplus::asio::object_server>&
                       objectServerArg) :
        objectPath(objectPathArg),
        objectServer(objectServerArg)
    {
    }

    template <class F>
    std::shared_ptr<sdbusplus::asio::dbus_interface>
        addInterface(const std::string& name, F&& function)
    {
        auto& interface = dbusInterfaces.emplace_back(
            objectServer->add_interface(objectPath, name),
            makeDbusInterfaceRemover(objectServer));
        function(**interface);
        (*interface)->initialize();

        return *interface;
    }

    template <class T>
    PropertyPtr<T> make_property_r(sdbusplus::asio::dbus_interface& interface,
                                   const std::string& name, const T& value)
    {
        return add_property(std::make_shared<Property<T>>(
            interface, name, value,
            sdbusplus::asio::PropertyPermission::readOnly));
    }

    template <class T>
    PropertyPtr<T>
        make_property_rw(sdbusplus::asio::dbus_interface& interface,
                         const std::string& name, const T& value,
                         std::function<void(void)> callback = nullptr)
    {
        return add_property(std::make_shared<Property<T>>(
            interface, name, value,
            sdbusplus::asio::PropertyPermission::readWrite, callback));
    }

  private:
    template <class T>
    std::shared_ptr<T> add_property(const std::shared_ptr<T>& property)
    {
        properties.emplace_back(property);
        return property;
    }

    const std::string& objectPath;
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::vector<DbusScopedInterface> dbusInterfaces;
    std::vector<std::shared_ptr<PropertyObject>> properties;
};

} // namespace nodemanager
