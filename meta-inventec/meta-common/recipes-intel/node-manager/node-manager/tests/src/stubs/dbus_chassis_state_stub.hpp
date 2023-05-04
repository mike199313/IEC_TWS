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

class DbusChassisStateStub : public DbusObjectStub
{
  public:
    DbusChassisStateStub(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
        DbusObjectStub(ioc, bus, objServer)
    {
        chassisIfaces.push_back(objServer->add_unique_interface(
            path(), interface(), [this](auto& iface) {
                iface.register_property_rw(
                    property.requestedPowerTransition(),
                    requestedPowerTransition.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, const auto&) {
                        return requestedPowerTransition.setValue(newValue);
                    },
                    [this](const auto&) {
                        return requestedPowerTransition.getValue();
                    });
            }));
        chassisIfaces.push_back(objServer->add_unique_interface(
            path(), interface(), [this](auto& iface) {
                iface.register_property_rw(
                    property.currentPowerState(), currentPowerState.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newValue, const auto&) {
                        return currentPowerState.setValue(newValue);
                    },
                    [this](const auto&) {
                        return currentPowerState.getValue();
                    });
            }));
    }

    virtual ~DbusChassisStateStub() = default;

    const char* path() override
    {
        return "/xyz/openbmc_project/state/chassis0";
    }
    const char* interface()
    {
        return "xyz.openbmc_project.State.Chassis";
    }

    struct Properties
    {
        static const char* requestedPowerTransition()
        {
            return "RequestedPowerTransition";
        }
        static const char* currentPowerState()
        {
            return "CurrentPowerState";
        }
    };

    static constexpr Properties property = {};
    testing::NiceMock<PropertyMock<std::string>> requestedPowerTransition{
        "xyz.openbmc_project.State.Chassis.Transition.On"};
    testing::NiceMock<PropertyMock<std::string>> currentPowerState{
        "xyz.openbmc_project.State.Chassis.PowerState.On"};

  private:
    std::vector<std::unique_ptr<sdbusplus::asio::dbus_interface>> chassisIfaces;
};
