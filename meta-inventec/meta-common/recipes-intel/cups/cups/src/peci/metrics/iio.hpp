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
#include "utils/log.hpp"

#include <boost/beast/core/span.hpp>

#include <cstdint>
#include <optional>
#include <set>
#include <sstream>

namespace cups
{

namespace peci
{

namespace metrics
{

enum class LinkSpeedId
{
    LinkSpeed2_5GbitsPerSecond = 1,
    LinkSpeed5GbitsPerSecond = 2,
    LinkSpeed8GbitsPerSecond = 3,
    LinkSpeed16GbitsPerSecond = 4
};

struct Link
{
    Link(abi::iio::Port port, abi::iio::Controller controller, uint8_t speedId,
         uint8_t width) :
        port(port),
        controller(controller), width(width)
    {
        LinkSpeedId id = static_cast<LinkSpeedId>(speedId);
        switch (id)
        {
            // Values are doubled because each link supports
            // Full-Duplex transmission
            case LinkSpeedId::LinkSpeed2_5GbitsPerSecond:
                speed = 2.5;
                break;
            case LinkSpeedId::LinkSpeed5GbitsPerSecond:
                speed = 5;
                break;
            case LinkSpeedId::LinkSpeed8GbitsPerSecond:
                speed = 8;
                break;
            case LinkSpeedId::LinkSpeed16GbitsPerSecond:
                speed = 16;
                break;
            default:
                LOG_ERROR << "Invalid Link speed id: "
                          << static_cast<unsigned>(id);
                speed = 0;
                break;
        }
    }

    std::string name() const
    {
        std::stringstream ss;
        ss << port.toString();
        ss << " / ";
        ss << controller.toString();
        return ss.str();
    }

    std::string str() const
    {
        const auto gbps = speed * width / 8;
        return name() + " x" + std::to_string(static_cast<int>(width)) + " " +
               std::to_string(speed) + " Gbit/s (" + std::to_string(gbps) +
               " GB/s)";
    }

    abi::iio::Port port;
    abi::iio::Controller controller;
    float speed;
    uint8_t width;
};

class Iio
{
  public:
    Iio(std::shared_ptr<peci::transport::Adapter> peciAdapterArg,
        uint8_t addressArg, uint32_t cpuIdArg, uint64_t maxUtilArg,
        std::vector<Link> linksArg) :
        peciAdapter{peciAdapterArg},
        address{addressArg}, cpuId{cpuIdArg}, maxUtil{maxUtilArg}, links{
                                                                       linksArg}
    {
        // Monitor each unique combination of :
        // - port bus number
        // - controller type
        //
        // It's done that way, because
        // - each port (bus) have sku-specific controllers
        // - each controller exposes multiple lanes/links (devices)
        // - performance counters are shared among  all 'devices' within single
        //   'bus' (port) for each controller.
        using Bus = uint8_t;
        using Controller = std::pair<Bus, abi::iio::Controller::Gen>;

        std::set<Controller> controllers;
        for (const auto& link : links)
        {
            const Bus bus = link.port.bus;
            const abi::iio::Controller::Gen gen = link.controller.gen;
            if (controllers.find({bus, gen}) == controllers.end())
            {
                controllers.insert({bus, gen});
                linksToMonitor.push_back(link);
            }
        }

        samples.resize(linksToMonitor.size());
    }

    void print() const
    {
        std::stringstream ss;
        for (const auto& link : links)
        {
            ss << "\n> " << link.str();
        }

        LOG_INFO << "IIO discovery data:"
                 << "\nAddress: 0x" << std::hex
                 << static_cast<unsigned>(address)
                 << "\nMax utilization (raw): " << std::dec << maxUtil
                 << " DWORDs (x4 bytes)"
                 << "\nMax utilization: " << std::dec
                 << maxUtil / peci::abi::iio::dwordsInGigabyte << " GB/s"
                 << "\nActive PCI links: " << links.size() << ss.str();
    }

    uint8_t getAddress() const
    {
        return address;
    }

    uint64_t getMaxUtil() const
    {
        return maxUtil;
    }

    std::optional<double> delta()
    {
        switch (cpu::toModel(cpuId))
        {
            case peci::cpu::model::spr:
                return accumulateIioPerfCounters();
                break;
            case peci::cpu::model::gnr:
                return accumulateIioFreePerfCounters();
                break;

            default:
                throw PECI_EXCEPTION_ADDR(address, "Unhandled");
        }
    }

    boost::beast::span<const Link> getLinks() const
    {
        return boost::beast::span<const Link>(links.data(), links.size());
    }

  private:
    std::shared_ptr<peci::transport::Adapter> peciAdapter;
    uint8_t address;
    uint32_t cpuId;
    uint64_t maxUtil;
    std::vector<Link> links;
    std::vector<Link> linksToMonitor;
    std::vector<CounterTracker<36>> samples{};

    std::optional<double> accumulateIioPerfCounters()
    {
        double deltaSum = 0;
        unsigned index = 0;

        for (const auto& link : linksToMonitor)
        {
            std::string tag = utils::toHex(getAddress()) + " " + link.name();
            uint32_t counterLow;
            uint32_t counterHigh;
            uint64_t counterCombined;

            const auto& [bus, type, _] = link.port;
            const auto& dev = link.controller.device;

            if (!peciAdapter->getXppMdl(address, bus, dev, counterLow))
            {
                return std::nullopt;
            }

            if (!peciAdapter->getXppMdh(address, bus, dev, counterHigh))
            {
                return std::nullopt;
            }

            counterCombined = counterLow;
            counterCombined |= static_cast<uint64_t>(counterHigh) << 32;

            auto& sample = samples[index];
            std::optional<double> delta = sample.getDelta(tag, counterCombined);
            if (delta)
            {
                deltaSum += *delta;
            }

            index++;
        }

        return deltaSum;
    }

    std::optional<double> accumulateIioFreePerfCounters()
    {
        for (const auto& link : linksToMonitor)
        {
            std::string tag = utils::toHex(getAddress()) + " " + link.name();
           
            const auto& [bus, type, _] = link.port;
            const auto& dev = link.controller.device;
            uint16_t offset = 0;
            uint64_t counterClk = 0;

            if (!peciAdapter->getXppMonFrCtrClk(address, bus, dev, counterClk))
            {
                return std::nullopt;
            }

            uint64_t counter0 = 0;
            if (!peciAdapter->getXppMonFrCtr(address, bus, dev, offset,
                                             counter0))
            {
                return std::nullopt;
            }

            uint64_t counter1 = 0;
            if (!peciAdapter->getXppMonFrCtr(address, bus, dev, offset + 8,
                                             counter1))
            {
                return std::nullopt;
            }

            uint64_t counter2 = 0;
            if (!peciAdapter->getXppMonFrCtr(address, bus, dev, offset + 16,
                                             counter2))
            {
                return std::nullopt;
            }

            uint64_t counter3 = 0;
            if (!peciAdapter->getXppMonFrCtr(address, bus, dev, offset + 24,
                                             counter3))
            {
                return std::nullopt;
            }
        }

        return 0;
    }
};

std::ostream& operator<<(std::ostream& o, const Iio& iio)
{
    o << std::showbase << std::hex << static_cast<unsigned>(iio.getAddress());
    return o;
}

} // namespace metrics

} // namespace peci

} // namespace cups
