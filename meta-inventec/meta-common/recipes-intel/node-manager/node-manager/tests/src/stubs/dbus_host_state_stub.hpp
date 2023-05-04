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

#include "dbus_object_stub.hpp"

class DbusHostStateStub : public DbusObjectStub
{
  public:
    DbusHostStateStub(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
        DbusObjectStub(ioc, bus, objServer)
    {
        hostIfce = objServer->add_unique_interface(
            path(), interface(), [this](auto& iface) {
                iface.register_property_rw(
                    property.requestedHostTransition(),
                    requestedHostTransition.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, const auto&) {
                        return requestedHostTransition.setValue(newValue);
                    },
                    [this](const auto&) {
                        return requestedHostTransition.getValue();
                    });
            });
    }

    virtual ~DbusHostStateStub() = default;

    const char* path() override
    {
        return "/xyz/openbmc_project/state/host0";
    }
    const char* interface()
    {
        return "xyz.openbmc_project.State.Host";
    }

    struct Properties
    {
        static const char* requestedHostTransition()
        {
            return "RequestedHostTransition";
        }
    };

    static constexpr Properties property = {};
    testing::NiceMock<PropertyMock<std::string>> requestedHostTransition{
        "xyz.openbmc_project.State.Host.Transition.On"};

  private:
    std::unique_ptr<sdbusplus::asio::dbus_interface> hostIfce;
};
