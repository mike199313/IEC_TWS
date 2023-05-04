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

#include "configuration.hpp"
#include "dbus/dbus.hpp"
#include "dbus/service.hpp"
#include "log.hpp"
#include "peci/transport/adapter.hpp"
#include "utils/traits.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>

constexpr auto defaultLogLevel = cups::utils::LogLevel::info;

namespace cups
{
class App
{
  public:
    App(boost::asio::io_context& iocArg) :
        ioc(iocArg), bus{std::make_shared<typeof(*bus)>(ioc)},
        objServer{std::make_shared<typeof(*objServer)>(bus)}, config(bus)
    {
        bus->request_name(dbus::Service);

        config.loadConfiguration(
            [this](const bool isEnabled, const Configuration::Values& values) {
                configureService(isEnabled, values);
            });
    }

  private:
    std::reference_wrapper<boost::asio::io_context> ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<dbus::CupsService> cupsService;
    std::shared_ptr<peci::transport::Adapter> peciAdapter;
    Configuration config;

    void configureService(const bool isEnabled,
                          const Configuration::Values& values)
    {
        if (!isEnabled)
        {
            LOG_INFO << "Sensor not configured. Service stopped";
            cupsService.reset();
            return;
        }

        if (!cupsService)
        {
            LOG_INFO << "Sensor configured at path: " << values.path;
            cupsService = dbus::CupsService::make(bus, objServer, values.path,
                                                  peciAdapter);
        }

        LOG_INFO << "(Config): interval: " << values.interval
                 << ", averagingPeriod: " << values.averagingPeriod
                 << ", loadFactorCfg: " << values.loadFactorCfg
                 << ", staticCoreLoadFactor: "
                 << values.staticLoadFactors.coreLoadFactor
                 << ", staticIioLoadFactor: "
                 << values.staticLoadFactors.iioLoadFactor
                 << ", staticMemoryLoadFactor: "
                 << values.staticLoadFactors.memoryLoadFactor;

        if (values.interval !=
            static_cast<uint64_t>(cupsService->getInterval().count()))
        {
            cupsService->setInterval(
                std::chrono::milliseconds(values.interval));
        }

        if (values.averagingPeriod !=
            static_cast<uint64_t>(cupsService->getAveragingPeriod().count()))
        {
            cupsService->setAveragingPeriod(
                std::chrono::milliseconds(values.averagingPeriod));
        }

        if (values.loadFactorCfg != cupsService->getLoadFactorCfg())
        {
            cupsService->setLoadFactorCfg(values.loadFactorCfg);
        }

        if (values.staticLoadFactors != cupsService->getStaticLoadFactors())
        {
            cupsService->setStaticLoadFactors(
                {values.staticLoadFactors.coreLoadFactor,
                 values.staticLoadFactors.iioLoadFactor,
                 values.staticLoadFactors.memoryLoadFactor});
        }
    }
};

} // namespace cups

int main(int argc, char* argv[])
{
    auto logLevel{cups::utils::toIntegral(defaultLogLevel)};

    CLI::App params("CUPS - CPU Utilization Per Second");

    auto option = params.add_option("-l,--loglevel", logLevel,
                                    "Lower = more verbose. Default is " +
                                        std::to_string(logLevel));
    option->check(CLI::Range(
        cups::utils::toIntegral(cups::utils::LogLevel::most_verbose),
        cups::utils::toIntegral(cups::utils::LogLevel::least_verbose)));

    CLI11_PARSE(params, argc, argv);

    cups::utils::logger::setLogLevel(
        static_cast<cups::utils::LogLevel>(logLevel));

    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait(
        [&ioc](const boost::system::error_code&, const int&) { ioc.stop(); });

    LOG_INFO << "Application starting";

    cups::App app(ioc);

    ioc.run();

    return 0;
}
