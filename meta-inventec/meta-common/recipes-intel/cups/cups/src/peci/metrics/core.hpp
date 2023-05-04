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
#include "peci/transport/adapter.hpp"
#include "utils/log.hpp"

#include <cstdint>
#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

class Core
{
  public:
    Core(std::shared_ptr<peci::transport::Adapter> peciAdapterArg,
         uint8_t addressArg, uint32_t cpuIdArg, uint64_t coreMaskArg,
         uint8_t coreCountArg, uint32_t maxNonTurboFreqArg,
         uint32_t maxTurboFreqArg, bool turboArg, uint8_t busNumberArg) :
        peciAdapter{peciAdapterArg},
        address{addressArg}, cpuId{cpuIdArg}, coreMask{coreMaskArg},
        coreCount{coreCountArg}, maxNonTurboFreq{maxNonTurboFreqArg},
        maxTurboFreq{maxTurboFreqArg}, turbo{turboArg}, busNumber{busNumberArg}
    {}

    void print() const
    {
        LOG_INFO << "Core discovery data:"
                 << "\nAddress: 0x" << std::hex
                 << static_cast<unsigned>(address) << "\nCPUID: 0x" << std::hex
                 << static_cast<unsigned>(cpuId) << "\nBus number: 0x"
                 << std::hex << static_cast<unsigned>(busNumber)
                 << "\nCore mask: 0x" << std::hex << coreMask
                 << "\nCore count: " << std::dec
                 << static_cast<unsigned>(coreCount)
                 << "\nFreq (non-turbo): " << std::dec
                 << static_cast<unsigned>(maxNonTurboFreq) << " MHz"
                 << "\nFreq (turbo): " << std::dec
                 << static_cast<unsigned>(maxTurboFreq) << " MHz"
                 << "\nTurbo enabled: " << turbo;
    }

    uint8_t getAddress() const
    {
        return address;
    }

    uint8_t getBusNumber() const
    {
        return busNumber;
    }

    uint8_t getCoreCount() const
    {
        return coreCount;
    }

    uint64_t getCoreMask() const
    {
        return coreMask;
    }

    uint32_t getCpuId() const
    {
        return cpuId;
    }

    uint32_t getMaxNonTurboFreq() const
    {
        return maxNonTurboFreq;
    }

    uint32_t getMaxTurboFreq() const
    {
        return maxTurboFreq;
    }

    bool getTurbo() const
    {
        return turbo;
    }

    uint64_t getMaxUtil() const
    {
        return coreCount *
               peci::abi::MHzToHz(turbo ? maxTurboFreq : maxNonTurboFreq);
    }

    std::optional<double> delta()
    {
        std::string tag = utils::toHex(getAddress()) + " C0 Residency";
        uint64_t counter = 0;

        if (!peciAdapter->getCpuC0Counter(address, counter))
        {
            return std::nullopt;
        }

        return sample.getDelta(tag, counter);
    }

  private:
    std::shared_ptr<peci::transport::Adapter> peciAdapter;
    uint8_t address;
    uint32_t cpuId;
    uint64_t coreMask;
    uint8_t coreCount;
    uint32_t maxNonTurboFreq;
    uint32_t maxTurboFreq;
    bool turbo;
    uint8_t busNumber;
    CounterTracker<64> sample;
};

std::ostream& operator<<(std::ostream& o, const Core& cpu)
{
    o << std::showbase << std::hex << static_cast<unsigned>(cpu.getAddress());
    return o;
}

} // namespace metrics

} // namespace peci

} // namespace cups
