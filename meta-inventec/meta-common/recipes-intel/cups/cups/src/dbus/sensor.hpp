/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2020 Intel Corporation
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you (License). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or
 *  transmit this software or the related documents without
 *  Intel's prior written permission.
 *
 *  This software and the related documents are provided as is,
 *  with no express or implied warranties, other than those
 *  that are expressly stated in the License.
 */

#pragma once

#include "base/sensor.hpp"
#include "dbus/dbus.hpp"
#include "log.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace cups
{

namespace dbus
{

class Sensor
{
  public:
    Sensor(std::shared_ptr<sdbusplus::asio::connection> busArg,
           std::shared_ptr<sdbusplus::asio::object_server> objServerArg,
           std::shared_ptr<base::Sensor> sensorArg,
           const std::string& chassisPathArg) :
        bus(busArg),
        objServer(objServerArg), sensor(sensorArg),
        path(dbus::open_bmc::SensorPath + sensor->getName()),
        chassisPath(chassisPathArg)
    {
        LOG_DEBUG_T(path) << "dbus::Sensor Constructor";

        setupSensor();
        setupAssociation();
    }

    ~Sensor()
    {
        LOG_DEBUG_T(path) << "dbus::Sensor ~Sensor";
        objServer->remove_interface(iface);
        objServer->remove_interface(association);
    }

    friend std::ostream& operator<<(std::ostream& os, const Sensor& r);

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;
    std::shared_ptr<base::Sensor> sensor;
    const std::string path;
    const std::string chassisPath;

    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;

    void setupSensor()
    {
        LOG_DEBUG << "Populating " << dbus::open_bmc::SensorIface;

        iface = objServer->add_interface(path, dbus::open_bmc::SensorIface);

        iface->register_property("Value", double(0));
        iface->register_property("MinValue", double(0));
        iface->register_property("MaxValue", double(100));
        iface->register_property("Unit", std::string_view("Percent").data());

        sensor->registerObserver(
            [name{sensor->getName()},
             iface{std::weak_ptr<typeof(*iface)>(iface)}](
                const boost::system::error_code& e, const double valueArg) {
                double value;

                if (e)
                {
                    LOG_ERROR_T(name) << "Read error: " << e;
                    value = std::numeric_limits<double>::quiet_NaN();
                }
                else
                {
                    value = valueArg;
                }

                if (auto sharedIface = iface.lock())
                {
                    LOG_DEBUG_T(name) << "New value: " << value;
                    sharedIface->set_property("Value", value);
                }
            });

        iface->initialize();
    }

    void setupAssociation()
    {
        LOG_DEBUG << "Populating " << dbus::open_bmc::AssociationIface;

        association =
            objServer->add_interface(path, dbus::open_bmc::AssociationIface);

        std::tuple<std::string, std::string, std::string> assoc = {
            "chassis", "all_sensors", chassisPath};

        association->register_property("Associations",
                                       std::vector<typeof(assoc)>{assoc});
        association->initialize();
    }
};

inline std::ostream& operator<<(std::ostream& os, const Sensor& s)
{
    os << s.sensor->getName();
    return os;
}

} // namespace dbus

} // namespace cups
