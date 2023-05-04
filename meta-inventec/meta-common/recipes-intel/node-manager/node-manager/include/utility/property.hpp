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

namespace nodemanager
{

class PropertyObject
{
  public:
    virtual ~PropertyObject() = default;
};

template <class T>
class Property : public PropertyObject
{
  public:
    Property(sdbusplus::asio::dbus_interface& interfaceArg,
             const std::string& nameArg, const T& valueArg,
             sdbusplus::asio::PropertyPermission permissionArg,
             std::function<void(void)> callbackArg = nullptr) :
        interface(interfaceArg),
        name(nameArg), value(valueArg)
    {
        register_property(permissionArg, callbackArg);
    }
    Property(const Property&) = delete;
    Property(Property&&) = delete;
    Property& operator=(const Property&) = delete;
    Property& operator=(Property&&) = delete;

    T get() const
    {
        return value;
    }

    void set(const T& newValue)
    {
        value = newValue;
        interface.set_property(name, newValue);
    }

  private:
    void register_property(sdbusplus::asio::PropertyPermission permission,
                           std::function<void(void)> callback)
    {
        switch (permission)
        {
            case sdbusplus::asio::PropertyPermission::readOnly: {
                interface.register_property(name, value, permission);
                break;
            }
            case sdbusplus::asio::PropertyPermission::readWrite: {
                interface.register_property(
                    name, value,
                    [this, callback](const T& newValue, T& property) {
                        value = property = newValue;
                        if (callback != nullptr)
                            callback();
                        return true;
                    },
                    [](const T& property) -> T { return property; });
                break;
            }
        }
    }

    sdbusplus::asio::dbus_interface& interface;
    std::string name;
    T value;
};

template <class T>
using PropertyPtr = std::shared_ptr<Property<T>>;

} // namespace nodemanager
