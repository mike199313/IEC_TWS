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

#include "peci/abi.hpp"
#include "peci/exception.hpp"
#include "peci/metrics/iio.hpp"

#include <boost/beast/core/span.hpp>

#include <cstdint>
#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

class IioFactory
{
  public:
    IioFactory(std::shared_ptr<peci::transport::Adapter> peciAdapterArg) :
        peciAdapter{peciAdapterArg}
    {}

    std::optional<Iio> detect(uint8_t address, uint32_t cpuId)
    {
        try
        {
            auto [maxUtil, links] = detectActiveLinks(address, cpuId);
            switch (cpu::toModel(cpuId))
            {
                case peci::cpu::model::spr:
                    configureXppCounters(address, links);
                    break;
                case peci::cpu::model::gnr:
                    break;

                default:
                    throw PECI_EXCEPTION_ADDR(address, "Unhandled");
            }

            return Iio(peciAdapter, address, cpuId, maxUtil, std::move(links));
        }
        catch (const peci::Exception& e)
        {
            LOG_ERROR << e.what();
            return std::nullopt;
        }
    }

  private:
    std::shared_ptr<peci::transport::Adapter> peciAdapter;

    std::tuple<uint64_t, std::vector<Link>>
        detectActiveLinks(uint8_t address, uint32_t cpuId) const
    {
        switch (cpu::toModel(cpuId))
        {
            case peci::cpu::model::spr:
                return detectActiveLinks(address, abi::iio::spr::linksMap);
            case peci::cpu::model::gnr:
                return detectActiveLinks(address, abi::iio::gnr::linksMap);

            default:
                throw PECI_EXCEPTION_ADDR(address, "Unhandled");
        }
    }

    template <typename LinkDescriptor, std::size_t N>
    std::tuple<uint64_t, std::vector<Link>>
        detectActiveLinks(uint8_t address,
                          const std::array<LinkDescriptor, N>& linksMap) const
    {
        uint64_t maxUtil = 0;
        std::vector<Link> links;

        for (auto [port, controller, reg] : linksMap)
        {
            uint8_t linkSpeedId;
            uint8_t linkWidth;
            uint8_t bus = port.bus;
            uint8_t dev = controller.device;
            bool active;

            if (cpu::isPrimary(address) && port.isDualUse)
            {
                port.type = abi::iio::Port::Type::DMI;

                // CPU0 reuses controller to connect to DMI (PCH PCI lane).
                // We need to filter out other links in this port, as they
                // falsively report link_status as 'Active'.
                if (!controller.canBeDmi)
                {
                    continue;
                }
            }

            LOG_DEBUG_T(utils::toHex(address))
                << "Trying to detect @ " << port.toString() << " / "
                << controller.toString();

            if (!peciAdapter->getLinkStatus(address, bus, dev, reg, linkSpeedId,
                                            linkWidth, active))
            {
                LOG_DEBUG << "Ignoring - some of the links can be disabled";
                continue;
            }

            if (active)
            {
                auto link = links.emplace_back(port, controller, linkSpeedId,
                                               linkWidth);
                maxUtil += static_cast<uint64_t>(link.speed * link.width *
                                                 abi::iio::fullDuplex);

                LOG_DEBUG << "Detected: " << link.str();
            }
        }

        // Convert to bytes per second
        maxUtil = (maxUtil * abi::iio::dwordsInGigabyte) / 8;

        return {maxUtil, std::move(links)};
    }

    void configureXppCounters(uint8_t address,
                              const std::vector<Link>& links) const
    {
        for (const auto& link : links)
        {
            const auto& [bus, type, _] = link.port;
            const auto& dev = link.controller.device;
            uint32_t xppMr = 0;
            uint32_t xppMer = 0;
            uint32_t xppErConf = 0;

            LOG_DEBUG_T(utils::toHex(address))
                << link.name() << " : Configuring XPP counters";

            if (!peciAdapter->getXppMr(address, bus, dev, xppMr))
            {
                throw PECI_EXCEPTION_ADDR(address, "getXppMr() failed");
            }

            if (!peciAdapter->getXppMer(address, bus, dev, xppMer))
            {
                throw PECI_EXCEPTION_ADDR(address, "getXppMer() failed");
            }

            if (!peciAdapter->getXppErConf(address, bus, dev, xppErConf))
            {
                throw PECI_EXCEPTION_ADDR(address, "getXppErConf() failed");
            }

            if (abi::iio::xppMr != xppMr)
            {
                LOG_DEBUG << "Updating XPPMR";
                if (!peciAdapter->setXppMr(address, bus, dev, abi::iio::xppMr))
                {
                    throw PECI_EXCEPTION_ADDR(address, "setXppMr() failed");
                }
            }

            if (abi::iio::xppMer != xppMer)
            {
                LOG_DEBUG << "Updating XPPMER";
                if (!peciAdapter->setXppMer(address, bus, dev,
                                            abi::iio::xppMer))
                {
                    throw PECI_EXCEPTION_ADDR(address, "setXppMer() failed");
                }
            }

            if (abi::iio::xppErConf != xppErConf)
            {
                LOG_DEBUG << "Updating XPPERCONF";
                if (!peciAdapter->setXppErConf(address, bus, dev,
                                               abi::iio::xppErConf))
                {
                    throw PECI_EXCEPTION_ADDR(address, "setXppErConf() failed");
                }
            }
        }
    }
};

} // namespace metrics

} // namespace peci

} // namespace cups
