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
#include "peci/metrics/utilization.hpp"
#include "utils/log.hpp"
#include "utils/traits.hpp"

#include <boost/container/flat_set.hpp>

#include <cstdint>
#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

class Memory
{
    struct Counters
    {
        CounterTracker<32> read;
        CounterTracker<32> write;
    };

  public:
    Memory(std::shared_ptr<peci::transport::Adapter> peciAdapterArg,
           uint8_t addressArg, uint32_t cpuIdArg, uint8_t cpuBusNumberArg,
           uint8_t dimmCountArg, uint8_t channelCountArg,
           uint32_t frequencyArg, uint64_t maxUtilArg) :
        peciAdapter{peciAdapterArg},
        address{addressArg}, cpuId{cpuIdArg},
        cpuBusNumber{cpuBusNumberArg}, dimmCount{dimmCountArg},
        channelCount{channelCountArg}, frequency{frequencyArg}, maxUtil{maxUtilArg}
    {}

    void print() const
    {
        LOG_INFO << "Memory discovery data:"
                 << "\nAddress: 0x" << std::hex
                 << static_cast<unsigned>(address)
                 << "\nDIMM count: " << std::dec
                 << static_cast<unsigned>(dimmCount)
                 << "\nChannel count: " << std::dec
                 << static_cast<unsigned>(channelCount)
                 << "\nFrequency: " << std::dec << frequency << " MHz"
                 << "\nMax util: " << std::dec << maxUtil << " x "
                 << abi::memory::cacheLineSize << "B";
    }

    uint8_t getAddress() const
    {
        return address;
    }

    uint8_t getCpuBusNumber() const
    {
        return cpuBusNumber;
    }

    uint8_t getChannelCount() const
    {
        return channelCount;
    }

    uint8_t getDimmCount() const
    {
        return dimmCount;
    }

    uint32_t getFrequency() const
    {
        return frequency;
    }

    uint64_t getMaxUtil() const
    {
        return maxUtil;
    }

    std::optional<double> delta()
    {
        return accumulateMemoryRwCounters();
    }

  private:
    std::shared_ptr<peci::transport::Adapter> peciAdapter;
    uint8_t address;
    uint32_t cpuId;
    uint8_t cpuBusNumber;
    uint8_t dimmCount;
    uint8_t channelCount;
    uint32_t frequency;
    uint64_t maxUtil;

    std::array<Counters, abi::memory::spr::controllersSampleIdxMap.size()>
        samples{};

    std::optional<double> accumulateMemoryRwCounters()
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                return accumulateMemoryRwCounters(
                    abi::memory::spr::controllersSampleIdxMap);
            case cpu::model::gnr:
                return accumulateMemoryRwCounters(
                    abi::memory::gnr::controllersSampleIdxMap);

            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }

    template <typename MemoryController, std::size_t N>
    std::optional<double> accumulateMemoryRwCounters(
        const std::array<MemoryController, N>& controllersSampleIdxMap)
    {
        static_assert(
            utils::std_array_size<decltype(controllersSampleIdxMap)> <=
                utils::std_array_size<decltype(samples)>,
            "Size mismatch");

        double deltaSum = 0;
        unsigned index = 0;
        for (const auto& [aggregatorIdx, sampleIdx] : controllersSampleIdxMap)
        {
            std::string tag = utils::toHex(getAddress()) + " Memory #" +
                              std::to_string(index);
            uint32_t rdCounter;
            uint32_t wrCounter;

            if (!peciAdapter->getMemoryRwCounters(
                    address, aggregatorIdx, sampleIdx, wrCounter, rdCounter))
            {
                return std::nullopt;
            }

            auto& sample = samples[index];
            std::optional<double> deltaRd =
                sample.read.getDelta(tag + " RD", rdCounter);
            std::optional<double> deltaWr =
                sample.write.getDelta(tag + " WR", wrCounter);
            if (deltaRd && deltaWr)
            {
                deltaSum += (*deltaRd + *deltaWr);
            }

            index++;
        }

        return deltaSum;
    }
};

std::ostream& operator<<(std::ostream& o, const Memory& memory)
{
    o << std::showbase << std::hex
      << static_cast<unsigned>(memory.getAddress());
    return o;
}

} // namespace metrics

} // namespace peci

} // namespace cups
