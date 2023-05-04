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

#include "log.hpp"

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/native_types.hpp>

namespace cups
{

namespace dbus
{

static constexpr auto Service = "xyz.openbmc_project.CupsService";
static constexpr auto Iface = "xyz.openbmc_project.CupsService";
static constexpr auto Path = "/xyz/openbmc_project/CupsService";

namespace open_bmc
{

static constexpr auto SensorIface = "xyz.openbmc_project.Sensor.Value";
static constexpr auto SensorPath = "/xyz/openbmc_project/sensors/utilization/";
static constexpr auto AssociationIface =
    "xyz.openbmc_project.Association.Definitions";

} // namespace open_bmc

std::string subObject(const std::string& name)
{
    return std::string(Service) + "." + name;
}

std::string subIface(const std::string& name)
{
    return std::string(Iface) + "." + name;
}

std::string subPath(const std::string& name)
{
    return std::string(Path) + "/" + name;
}

static inline void throwError(const std::errc e, const std::string& msg)
{
    LOG_ERROR << "API error: " << std::make_error_code(e) << " : " << msg;
    throw sdbusplus::exception::SdBusError(static_cast<int>(e), msg.c_str());
}

} // namespace dbus

} // namespace cups
