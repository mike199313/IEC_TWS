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

#include "dbus_object_stub.hpp"

class DbusPldmEffecterStub : public DbusObjectStub
{
  public:
    DbusPldmEffecterStub(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
        const std::string& deviceName, const std::string& tid,
        const DeviceIndex deviceIndex, const std::string& effecterType,
        const std::string& effecterName) :
        DbusObjectStub(ioc, bus, objServer)
    {
        objectPath = "/xyz/openbmc_project/pldm/" + tid + "/effecter/" +
                     effecterType + "/PCIe_Slot_" +
                     std::to_string(deviceIndex + 1) + "_" + deviceName + "_" +
                     effecterName;

        pldmEntityIfaces.push_back(objServer->add_unique_interface(
            path(), valueInterface(), [this](auto& iface) {
                iface.register_property_r(
                    property.value(), value.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return value.getValue(); });
                iface.register_property_r(
                    property.minValue(), minValue.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return minValue.getValue(); });
                iface.register_property_r(
                    property.maxValue(), maxValue.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return maxValue.getValue(); });
            }));

        pldmEntityIfaces.push_back(objServer->add_unique_interface(
            path(), operationalStatusInterface(), [this](auto& iface) {
                iface.register_property_r(
                    property.functional(), functional.getValue(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto&) { return functional.getValue(); });
            }));

        pldmEntityIfaces.push_back(objServer->add_unique_interface(
            path(), setNumericEffecterInterface(), [this](auto& iface) {
                iface.register_method(method.setEffecter(),
                                      [this](double newValue) {
                                          setEffecter(newValue);
                                          if (newValue < minValue.getValue() ||
                                              newValue > maxValue.getValue())
                                          {
                                              return false;
                                          }
                                          return value.setValue(newValue);
                                      });
            }));
    }

    virtual ~DbusPldmEffecterStub() = default;

    const char* path() override
    {
        return objectPath.c_str();
    }
    const char* valueInterface()
    {
        return "xyz.openbmc_project.Effecter.Value";
    }
    const char* operationalStatusInterface()
    {
        return "xyz.openbmc_project.State.Decorator.OperationalStatus";
    }
    const char* setNumericEffecterInterface()
    {
        return "xyz.openbmc_project.Effecter.SetNumericEffecter";
    }

    struct Properties
    {
        static const char* value()
        {
            return "Value";
        }
        static const char* minValue()
        {
            return "MinValue";
        }
        static const char* maxValue()
        {
            return "MaxValue";
        }
        static const char* functional()
        {
            return "Functional";
        }
    };

    struct Methods
    {
        static const char* setEffecter()
        {
            return "SetEffecter";
        }
    };

    static constexpr Properties property = {};
    testing::NiceMock<PropertyMock<double>> value{0.0};
    testing::NiceMock<PropertyMock<double>> minValue{0.0};
    testing::NiceMock<PropertyMock<double>> maxValue{32767.0};
    testing::NiceMock<PropertyMock<bool>> functional{true};

    static constexpr Methods method = {};
    MOCK_METHOD(void, setEffecter, (double), ());

  private:
    std::vector<std::unique_ptr<sdbusplus::asio::dbus_interface>>
        pldmEntityIfaces;
    std::string objectPath;
};
