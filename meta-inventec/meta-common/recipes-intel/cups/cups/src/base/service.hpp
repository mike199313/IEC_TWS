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

#include "base/discovery.hpp"
#include "base/readings.hpp"
#include "base/sensor.hpp"
#include "peci/transport/adapter.hpp"

#include <boost/beast/core/span.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <algorithm>
#include <functional>
#include <memory>

namespace cups
{

namespace base
{

class CupsService
{
    // Prevents constructor from being called externally
    struct ctor_lock
    {};

  public:
    CupsService(ctor_lock, boost::asio::io_context& iocArg,
                std::shared_ptr<peci::transport::Adapter> peciAdapterArg) :
        ioc(iocArg),
        peciAdapter(peciAdapterArg)
    {}

    static std::shared_ptr<CupsService>
        make(boost::asio::io_context& ioc,
             std::shared_ptr<sdbusplus::asio::connection> bus,
             std::shared_ptr<peci::transport::Adapter> peciAdapter)
    {
        auto cupsService =
            std::make_shared<CupsService>(ctor_lock{}, ioc, peciAdapter);

        createSensorsAndRegisterCallbacks(cupsService, ioc, bus);

        return cupsService;
    }

    const boost::beast::span<std::shared_ptr<Sensor>> getSensors()
    {
        return boost::beast::span<std::shared_ptr<Sensor>>(sensors.data(),
                                                           sensors.size());
    }

    std::chrono::milliseconds getInterval() const
    {
        return readings->getInterval();
    }

    void setInterval(std::chrono::milliseconds interval)
    {
        readings->setInterval(interval);
    }

    std::chrono::milliseconds getAveragingPeriod() const
    {
        return readings->getAveragingPeriod();
    }

    void setAveragingPeriod(std::chrono::milliseconds averagingPeriod)
    {
        readings->setAveragingPeriod(averagingPeriod);
    }

    const LoadFactorCfg& getLoadFactorCfg() const
    {
        return readings->getLoadFactorCfg();
    }

    void setLoadFactorCfg(const LoadFactorCfg& loadFactorCfg)
    {
        readings->setLoadFactorCfg(loadFactorCfg);
    }

    const LoadFactors& getStaticLoadFactors() const
    {
        return readings->getStaticLoadFactors();
    }

    void setStaticLoadFactors(double coreLoadFactor, double iioLoadFactor,
                              double memoryLoadFactor)
    {
        readings->setStaticLoadFactors(coreLoadFactor, iioLoadFactor,
                                       memoryLoadFactor);
    }

    const LoadFactors& getDynamicLoadFactors() const
    {
        return readings->getDynamicLoadFactors();
    }

  private:
    static void createSensorsAndRegisterCallbacks(
        std::shared_ptr<CupsService> cupsService, boost::asio::io_context& ioc,
        std::shared_ptr<sdbusplus::asio::connection> bus)
    {
        createReadingsAndDiscoveryCallbacks(cupsService, ioc, bus);
        createAndRegisterSensors(cupsService);
    }

    static void createReadingsAndDiscoveryCallbacks(
        std::shared_ptr<CupsService> cupsService, boost::asio::io_context& ioc,
        std::shared_ptr<sdbusplus::asio::connection> bus)
    {
        auto cpuSetter = [weakCupsService =
                              std::weak_ptr<CupsService>(cupsService)](
                             const std::array<std::optional<peci::metrics::Cpu>,
                                              peci::cpu::limit>& cpuData) {
            if (auto service = weakCupsService.lock())
            {
                LOG_DEBUG << "Setting CPU data";
                service->setCpuData(cpuData);
            }
        };

        auto cpuGetter =
            [weakCupsService = std::weak_ptr<CupsService>(cupsService)](
                std::array<std::optional<peci::metrics::Cpu>, peci::cpu::limit>&
                    cpuData) {
                if (auto service = weakCupsService.lock())
                {
                    LOG_DEBUG << "Getting CPU data";
                    cpuData = service->cpus;
                }
            };

        cupsService->discovery = CupsDiscovery::make(
            ioc, cupsService->peciAdapter, std::move(cpuSetter));
        cupsService->readings =
            CupsReadings::make(ioc, bus, std::move(cpuGetter));
    }

    static void
        createAndRegisterSensors(std::shared_ptr<CupsService> cupsService)
    {
        auto sensorFailureCallback =
            [weakDiscovery = std::weak_ptr(cupsService->discovery)]() {
                if (auto discovery = weakDiscovery.lock())
                {
                    discovery->restartDiscovery();
                }
            };

        auto core = cupsService->sensors.emplace_back(std::make_shared<Sensor>(
            "HostCpuUtilization", sensorFailureCallback));
        auto iio = cupsService->sensors.emplace_back(std::make_shared<Sensor>(
            "HostPciBandwidthUtilization", sensorFailureCallback));
        auto memory =
            cupsService->sensors.emplace_back(std::make_shared<Sensor>(
                "HostMemoryBandwidthUtilization", sensorFailureCallback));
        auto cupsIndex = cupsService->sensors.emplace_back(
            std::make_shared<Sensor>("CupsIndex", sensorFailureCallback));

        auto averageCore =
            cupsService->sensors.emplace_back(AverageSensor::make(
                "AverageHostCpuUtilization", *core, sensorFailureCallback));
        auto averageIio = cupsService->sensors.emplace_back(AverageSensor::make(
            "AverageHostPciBandwidthUtilization", *iio, sensorFailureCallback));
        auto averageMemory = cupsService->sensors.emplace_back(
            AverageSensor::make("AverageHostMemoryBandwidthUtilization",
                                *memory, sensorFailureCallback));
        auto averageCupsIndex =
            cupsService->sensors.emplace_back(AverageSensor::make(
                "AverageCupsIndex", *cupsIndex, sensorFailureCallback));

        cupsService->readings->monitor(CupsReadings::Type::Core, core);
        cupsService->readings->monitor(CupsReadings::Type::Iio, iio);
        cupsService->readings->monitor(CupsReadings::Type::Memory, memory);
        cupsService->readings->monitor(CupsReadings::Type::CupsIndex,
                                       cupsIndex);
        cupsService->readings->monitor(CupsReadings::Type::AverageCore,
                                       averageCore);
        cupsService->readings->monitor(CupsReadings::Type::AverageIio,
                                       averageIio);
        cupsService->readings->monitor(CupsReadings::Type::AverageMemory,
                                       averageMemory);
        cupsService->readings->monitor(CupsReadings::Type::AverageCupsIndex,
                                       averageCupsIndex);
    }

    void setCpuData(const std::array<std::optional<peci::metrics::Cpu>,
                                     peci::cpu::limit>& cpuData)
    {
        const auto predicate =
            [](const std::optional<peci::metrics::Cpu>& cpu) {
                return cpu != std::nullopt;
            };

        const int was = std::count_if(cpus.begin(), cpus.end(), predicate);
        const int is = std::count_if(cpuData.begin(), cpuData.end(), predicate);

        if (was != is)
        {
            LOG_INFO << "CPU configuration change detected. CPU count = " << is;
            for (auto& cpu : cpuData)
            {
                if (cpu)
                {
                    cpu->core.print();
                    cpu->iio.print();
                    cpu->memory.print();
                }
            }
        }

        cpus = cpuData;
    }

    std::reference_wrapper<boost::asio::io_context> ioc;
    std::shared_ptr<peci::transport::Adapter> peciAdapter;
    std::vector<std::shared_ptr<Sensor>> sensors;

    std::shared_ptr<CupsDiscovery> discovery;
    std::shared_ptr<CupsReadings> readings;

    std::array<std::optional<peci::metrics::Cpu>, peci::cpu::limit> cpus;

    CupsService(const CupsService&) = delete;
    CupsService& operator=(const CupsService&) = delete;
};

} // namespace base

} // namespace cups
