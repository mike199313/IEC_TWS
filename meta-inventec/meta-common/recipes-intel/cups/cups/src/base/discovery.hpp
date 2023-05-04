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

#include "peci/metrics/types.hpp"
#include "utils/configuration.hpp"
#include "utils/log.hpp"

#include <boost/asio/steady_timer.hpp>

#include <array>
#include <chrono>
#include <functional>
#include <memory>

namespace cups
{

namespace base
{

class CupsDiscovery : public std::enable_shared_from_this<CupsDiscovery>
{
    // Prevents constructor from being called externally
    struct ctor_lock
    {};

  public:
    CupsDiscovery(
        ctor_lock, boost::asio::io_context& iocArg,
        std::shared_ptr<peci::transport::Adapter> peciAdapterArg,
        std::function<void(const std::array<std::optional<peci::metrics::Cpu>,
                                            peci::cpu::limit>&)>&& cpuUpdate) :
        ioc(iocArg),
        peciAdapter(peciAdapterArg),
        timer(iocArg, interval), updateCb{std::move(cpuUpdate)}
    {
        if (updateCb == nullptr)
        {
            throw std::runtime_error("Null cpuUpdate");
        }
    }

    static std::shared_ptr<CupsDiscovery> make(
        boost::asio::io_context& ioc,
        std::shared_ptr<peci::transport::Adapter> peciAdapter,
        std::function<void(const std::array<std::optional<peci::metrics::Cpu>,
                                            peci::cpu::limit>&)>&& cpuUpdate)
    {
        auto discovery = std::make_shared<CupsDiscovery>(
            ctor_lock{}, ioc, peciAdapter, std::move(cpuUpdate));
        discovery->detectCpus();
        discovery->updateCb(discovery->cpus);
        discovery->tick(discovery->interval);

        return discovery;
    }

    std::chrono::milliseconds getInterval() const
    {
        return interval;
    }

    void restartDiscovery()
    {
        tick(Configuration::intervalRange.min / 2);
    }

  private:
    std::reference_wrapper<boost::asio::io_context> ioc;
    std::shared_ptr<peci::transport::Adapter> peciAdapter;

    const std::chrono::milliseconds interval = std::chrono::seconds(30);
    boost::asio::steady_timer timer;

    CupsDiscovery(const CupsDiscovery&) = delete;
    CupsDiscovery& operator=(const CupsDiscovery&) = delete;

    std::array<std::optional<peci::metrics::Cpu>, peci::cpu::limit> cpus;
    std::function<void(
        const std::array<std::optional<peci::metrics::Cpu>, peci::cpu::limit>&)>
        updateCb;

    void tick(std::chrono::milliseconds startDelay)
    {
        timer.expires_after(startDelay);
        timer.async_wait(
            [self = shared_from_this()](const boost::system::error_code& e) {
                LOG_DEBUG << "CupsDiscovery::tick()";

                if (e)
                {
                    LOG_ERROR << "Timer failed with error : " << e.message();
                    return;
                }

                self->detectCpus();
                self->updateCb(self->cpus);
                self->tick(self->interval);
            });
    }

    void detectCpus()
    {
        LOG_DEBUG << "Detecting CPUs";

        for (size_t index = 0; index < peci::cpu::limit; index++)
        {
            cpus[index].reset();

            const uint8_t address =
                static_cast<uint8_t>(peci::cpu::minAddress + index);

            if (auto metrics = peci::metrics::Cpu::detect(peciAdapter, address))
            {
                LOG_DEBUG << "CPU found: " << metrics->core;
                cpus[index].emplace(std::move(*metrics));
            }
        }
    }
};

} // namespace base

} // namespace cups
