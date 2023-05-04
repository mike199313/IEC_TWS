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

#pragma once

template <typename T>
class ArbitraryDbusStub
{
  public:
    ArbitraryDbusStub(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
        std::string const& objectPath,
        const std::unordered_map<std::string, std::vector<std::string>>&
            interfacesAndProperties) :
        ioc(ioc),
        bus(bus), objServer(objServer)
    {
        for (auto [interfaceName, propertiesList] : interfacesAndProperties)
        {
            hostIfce = objServer->add_unique_interface(
                objectPath, interfaceName,
                [this, &propertiesList](auto& iface) {
                    for (auto& property : propertiesList)
                    {
                        properties.insert({property, ""});
                        iface.register_property_rw(
                            property, T(),
                            sdbusplus::vtable::property_::emits_change,
                            [this, property](const auto& newValue,
                                             auto& oldValue) {
                                properties.at(property) = oldValue = newValue;
                                return 1;
                            },
                            [this, property](const auto&) {
                                return properties.at(property);
                            });
                    }
                });
        }
    }

    virtual ~ArbitraryDbusStub() = default;

  private:
    std::unique_ptr<sdbusplus::asio::dbus_interface> hostIfce;
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unordered_map<std::string, T> properties;
};
