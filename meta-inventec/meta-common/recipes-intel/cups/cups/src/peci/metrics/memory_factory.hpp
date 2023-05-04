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

#include "peci/exception.hpp"
#include "peci/metrics/memory.hpp"

#include <boost/container/flat_set.hpp>

#include <cstdint>
#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

class MemoryFactory
{
  public:
    MemoryFactory(std::shared_ptr<peci::transport::Adapter> peciAdapterArg) :
        peciAdapter{peciAdapterArg}
    {}

    std::optional<Memory> detect(uint8_t address, uint32_t cpuId,
                                 uint8_t cpuBusNumber)
    {
        try
        {
            auto [dimmCount, channelCount] =
                detectDimmPopulation(address, cpuId, cpuBusNumber);
            uint32_t frequency = detectFrequency(address, cpuId, cpuBusNumber);
            uint64_t maxUtil = calcMaxUtil(channelCount, frequency);

            return Memory(peciAdapter, address, cpuId, cpuBusNumber, dimmCount,
                          channelCount, frequency, maxUtil);
        }
        catch (const peci::Exception& e)
        {
            LOG_ERROR << e.what();
            return std::nullopt;
        }
    }

  private:
    std::shared_ptr<peci::transport::Adapter> peciAdapter;

    std::tuple<uint8_t, uint8_t>
        detectDimmPopulation(uint8_t address, uint32_t cpuId,
                             uint8_t cpuBusNumber) const
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                return detectDimmPopulation(address, cpuId, cpuBusNumber,
                                            abi::memory::spr::channelsMap);
            case cpu::model::gnr:
                return detectDimmPopulation(address, cpuId, cpuBusNumber,
                                            abi::memory::gnr::channelsMap);

            default:
                throw PECI_EXCEPTION_ADDR(address, "Unhandled");
        }
    }

    template <typename MemoryDescriptor, std::size_t N>
    std::tuple<uint8_t, uint8_t> detectDimmPopulation(
        uint8_t address, uint32_t cpuId, uint8_t cpuBusNumber,
        const std::array<MemoryDescriptor, N>& channelsMap) const
    {
        uint8_t dimmCount = 0;
        boost::container::flat_set<uint8_t> channels;
        channels.reserve(channelsMap.size() / 2);

        for (const auto& [dev, func, reg, channel] : channelsMap)
        {
            bool dimmPopulated;

            if (!peciAdapter->isDIMMPopulated(
                    address, cpuId, cpuBusNumber, static_cast<uint8_t>(dev),
                    func, static_cast<uint32_t>(reg), dimmPopulated))
            {
                throw PECI_EXCEPTION_ADDR(address, "isDIMMPopulated() failed");
            }

            if (dimmPopulated)
            {
                channels.insert(channel);
                dimmCount++;
            }
        }

        return {dimmCount, static_cast<uint8_t>(channels.size())};
    }

    uint32_t detectFrequency(uint8_t address, uint32_t cpuId,
                             uint8_t cpuBusNumber) const
    {
        uint32_t frequency = 0;

        if (!peciAdapter->getMemoryFreq(address, cpuId, frequency))
        {
            throw PECI_EXCEPTION_ADDR(address, "getMemoryFreq() failed");
        }

        return frequency;
    }

    constexpr uint64_t calcMaxUtil(uint8_t channelCount,
                                   uint32_t frequency) const
    {
        return (channelCount * peci::abi::MHzToHz(frequency) *
                peci::abi::memory::channelWidth) /
               peci::abi::memory::cacheLineSize;
    }
};

} // namespace metrics

} // namespace peci

} // namespace cups
