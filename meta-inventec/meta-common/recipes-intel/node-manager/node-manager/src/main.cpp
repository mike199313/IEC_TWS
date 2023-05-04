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

#include "common_types.hpp"
#include "loggers/log.hpp"
#include "node_manager.hpp"
#include "sps_integrator.hpp"

#include <systemd/sd-daemon.h>

#include <iostream>
#include <sdbusplus/asio/object_server.hpp>

int main()
{
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM, SIGABRT);
    auto bus = std::make_shared<sdbusplus::asio::connection>(ioc);

    nodemanager::ThrottlingLogger::logRestart();
    if (!nodemanager::SpsIntegrator(bus).shouldNmStart())
    {
        sd_notify(0, "READY=1");
        sd_notify(0, "STOPPING=1");
        return 0;
    }
    auto objPath = std::string(nodemanager::kRootObjectPath);
    sdbusplus::server::manager::manager objManager(*bus.get(), objPath.c_str());
    nodemanager::NodeManager nodeManager(ioc, bus, objPath);
    signals.async_wait([&ioc, &nodeManager](const boost::system::error_code& ec,
                                            const int& code) {
        sd_notify(0, "STOPPING=1");
        if (code == SIGABRT)
        {
            nodemanager::Logger::log<nodemanager::LogLevel::info>(
                "Catching abort signal from WD, closing gracefully with status "
                "dump");
            nlohmann::json out;
            nodeManager.getDiagnostics()->reportStatus(out);
            nodemanager::Logger::log<nodemanager::LogLevel::info>(out.dump());
        }
        ioc.stop();
    });
    bus->request_name("xyz.openbmc_project.NodeManager");
    nodeManager.run();
    sd_notify(0, "READY=1");

    ioc.run();

    return 0;
}
