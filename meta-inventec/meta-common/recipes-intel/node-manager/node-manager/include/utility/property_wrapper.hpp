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

#include "types.hpp"

#include <functional>
#include <memory>

namespace nodemanager
{

template <class T>
class CustomPropertyGetter
{
  public:
    virtual ~CustomPropertyGetter() = default;

    /**
     * @brief Set the Custom Getter object, which will be called by internal
     * get() method or dbus getter. This variant accepts function without
     * arguments.
     *
     * @param customGetterArg
     */
    void setCustomGetter(std::function<T()> customGetterArg)
    {
        customGetter = [customGetterArg](const T& val) {
            return customGetterArg();
        };
    }

    /**
     * @brief Set the Custom Getter object, which will be called by internal
     * get() method or dbus getter. This variant accept function with single
     * argument, reference to wrapped value.
     *
     * @param customGetterArg
     */
    void setCustomGetter(std::function<T(const T& val)> customGetterArg)
    {
        customGetter = customGetterArg;
    }

  protected:
    std::optional<std::function<T(const T&)>> customGetter;
};

/**
 * @brief Wrapper for dbus property. Generates dbus getter and setter. Support
 * custom verification functions, custom getter function and custom post set
 * function.
 * @tparam T - wrapped type.
 */
template <class T>
class PropertyWrapper : public CustomPropertyGetter<T>
{
  public:
    PropertyWrapper() = default;
    PropertyWrapper(const PropertyWrapper&) = delete;
    PropertyWrapper(PropertyWrapper&&) = delete;
    PropertyWrapper& operator=(const PropertyWrapper&) = delete;
    PropertyWrapper& operator=(PropertyWrapper&&) = delete;

    /**
     * @brief get wrapped value or the result of custom getter function which
     * can be set by calling setCustomGetter() method.
     *
     * @return T
     */
    T get() const
    {
        if (CustomPropertyGetter<T>::customGetter)
        {
            return (*CustomPropertyGetter<T>::customGetter)(value);
        }
        else
        {
            return value;
        }
    }

    /**
     * @brief Set wrapped property to given value. This method checks if value
     * is different than current value. If property was registered on dbus by
     * calling registerProperty(), change signal will be emited after setting
     * the value.
     *
     * @param newVal - can be of same type as the one wrapped by
     * this class or of underlying type in case of enums.
     */
    template <class U>
    void set(const U& newVal)
    {
        if constexpr (std::is_same_v<utility::simple_type_t<T>, U>)
        {
            if (static_cast<utility::simple_type_t<T>>(value) != newVal)
            {
                value = static_cast<T>(newVal);
                if (dbusInterface)
                {
                    (*dbusInterface).get().signal_property(propertyName);
                }
            }
        }
        else if constexpr (std::is_same_v<typename utility::DbusPropertyTraits<
                                              T>::DbusType,
                                          U>)
        {
            T valueConverted =
                utility::DbusPropertyTraits<T>::fromDbusType(newVal);
            return set(valueConverted);
        }
        else
        {
            static_assert(std::is_same_v<U, T>,
                          "Property setter accepts only values of same "
                          "type or underlying type in case of enum");
            return set(static_cast<utility::simple_type_t<T>>(newVal));
        }
    }

    /**
     * @brief Set the verify function, which will be called by dbus setter. The
     * verify function should throw exception of type sdbusplus::exception_t (or
     * derived) when value is invalid.
     *
     * @param verifyDbusSetFunctionArg
     */
    void setVerifyDbusSetFunction(
        std::function<void(T)> verifyDbusSetFunctionArg)
    {
        verifyDbusSetFunction = verifyDbusSetFunctionArg;
    }

    /**
     * @brief Set the post set function, which will be called by dbus setter.
     *
     * @param postDbusSetFunctionArg
     */
    void setPostDbusSetFunction(std::function<void()> postDbusSetFunctionArg)
    {
        postDbusSetFunction = postDbusSetFunctionArg;
    }

    static T verifyFromDbusType(
        typename utility::DbusPropertyTraits<T>::DbusType value)
    {
        return utility::DbusPropertyTraits<T>::fromDbusType(value);
    }

    /**
     * @brief Register property on dbus, and create getter and setter methods,
     * which functionality may change if setVerifyDbusSetFunction(),
     * setPostDbusSetFunction() or setCustomGetter() were called. For additional
     * information, see get() and set() method description.
     *
     * @param dbusInterfaceArg
     * @param propertyNameArg
     * @param propertyPermission
     */
    void
        registerProperty(sdbusplus::asio::dbus_interface& dbusInterfaceArg,
                         const char* propertyNameArg,
                         sdbusplus::asio::PropertyPermission propertyPermission)
    {
        if (dbusInterface)
        {
            return;
        }
        dbusInterface = dbusInterfaceArg;
        propertyName = propertyNameArg;

        switch (propertyPermission)
        {
            case sdbusplus::asio::PropertyPermission::readWrite: {
                (*dbusInterface)
                    .get()
                    .register_property_rw(
                        propertyName,
                        utility::DbusPropertyTraits<T>::toDbusType(value),
                        sdbusplus::vtable::property_::emits_change,
                        [this](const auto& newVal, auto& oldValue) {
                            T newValConverted =
                                utility::DbusPropertyTraits<T>::fromDbusType(
                                    newVal);

                            if (value != newValConverted)
                            {
                                if (verifyDbusSetFunction)
                                {
                                    (*verifyDbusSetFunction)(newValConverted);
                                }
                                oldValue =
                                    utility::DbusPropertyTraits<T>::toDbusType(
                                        value = newValConverted);
                                if (postDbusSetFunction)
                                {
                                    (*postDbusSetFunction)();
                                }
                            }
                            return true;
                        },
                        [this](const auto&) {
                            return utility::DbusPropertyTraits<T>::toDbusType(
                                get());
                        });
                break;
            }
            case sdbusplus::asio::PropertyPermission::readOnly: {
                (*dbusInterface)
                    .get()
                    .register_property_r(
                        propertyName,
                        utility::DbusPropertyTraits<T>::toDbusType(value),
                        sdbusplus::vtable::property_::emits_change,
                        [this](const auto&) {
                            return utility::DbusPropertyTraits<T>::toDbusType(
                                get());
                        });
                break;
            }
        }
    }

  private:
    T value = T{};
    std::string propertyName;
    std::optional<std::reference_wrapper<sdbusplus::asio::dbus_interface>>
        dbusInterface;
    std::optional<std::function<void(T)>> verifyDbusSetFunction;
    std::optional<std::function<void()>> postDbusSetFunction;
};

/**
 * @brief Wrapper for const dbus property. Generates dbus getter and setter.
 * Support custom getter function.
 * @tparam T - wrapped type.
 */
template <class T>
class ConstProperty : public CustomPropertyGetter<T>
{
  public:
    ConstProperty(const T& val) : value(val)
    {
    }
    ConstProperty(const ConstProperty&) = delete;
    ConstProperty(ConstProperty&&) = delete;
    ConstProperty& operator=(const ConstProperty&) = delete;
    ConstProperty& operator=(ConstProperty&&) = delete;

    /**
     * @brief get wrapped value or the result of custom getter function which
     * can be set by calling setCustomGetter() method.
     *
     * @return T
     */
    T get() const
    {
        if (CustomPropertyGetter<T>::customGetter)
        {
            return (*CustomPropertyGetter<T>::customGetter)(value);
        }
        else
        {
            return value;
        }
    }

    /**
     * @brief Register property on dbus, and create getter method, which
     * functionality may change if setCustomGetter() was called. For addtional
     * information, see get() method description.
     *
     * @param dbusInterfaceArg
     * @param propertyNameArg
     */
    void registerProperty(sdbusplus::asio::dbus_interface& dbusInterfaceArg,
                          const char* propertyNameArg)
    {
        if (dbusInterface)
        {
            return;
        }
        dbusInterface = dbusInterfaceArg;
        propertyName = propertyNameArg;

        (*dbusInterface)
            .get()
            .register_property_r(
                propertyName, static_cast<utility::simple_type_t<T>>(value),
                sdbusplus::vtable::property_::const_, [this](const auto&) {
                    return static_cast<utility::simple_type_t<T>>(get());
                });
    }

  private:
    T value;
    std::string propertyName;
    std::optional<std::reference_wrapper<sdbusplus::asio::dbus_interface>>
        dbusInterface;
};

} // namespace nodemanager
