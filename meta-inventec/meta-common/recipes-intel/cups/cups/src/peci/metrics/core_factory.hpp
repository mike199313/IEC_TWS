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
#include "peci/metrics/core.hpp"

#include <bitset>
#include <cstdint>
#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

class CoreFactory
{
  public:
    CoreFactory(std::shared_ptr<peci::transport::Adapter> peciAdapterArg) :
        peciAdapter{peciAdapterArg}
    {}

    std::optional<Core> detect(uint8_t address)
    {
        uint32_t cpuId;

        // Initial communication
        try
        {
            cpuId = detectCpuId(address);
        }
        catch (const peci::Exception& e)
        {
            // Use debug not to flood logs with errors whenever CPU is absent
            // during discovery
            LOG_DEBUG << e.what();
            return std::nullopt;
        }

        // Validate CPUID
        try
        {
            cpu::toModel(cpuId);
        }
        catch (const peci::Exception& e)
        {
            LOG_CRITICAL << "Unsupported CPUID " << utils::toHex(cpuId)
                         << " at address " << utils::toHex(address)
                         << " . Make sure you run code on supported CPU.";
            return std::nullopt;
        }

        // Retrieve necessary values
        try
        {
            auto [coreMask, coreCount] = detectCores(address, cpuId);
            uint32_t maxNonTurboFreq = detectMaxNonTurboFreq(address, cpuId);
            uint32_t maxTurboFreq = detectMaxTurboFreq(address, coreCount);
            bool turbo = detectTurboValue(address, cpuId);
            uint8_t busNumber = detectBusNumber(address, cpuId);

            return Core(peciAdapter, address, cpuId, coreMask, coreCount,
                        maxNonTurboFreq, maxTurboFreq, turbo, busNumber);
        }
        catch (const peci::Exception& e)
        {
            LOG_ERROR << e.what();
            return std::nullopt;
        }
    }

  private:
    std::shared_ptr<peci::transport::Adapter> peciAdapter;

    uint32_t detectCpuId(uint8_t address)
    {
        uint32_t cpuId;

        if (!peciAdapter->getCpuId(address, cpuId))
        {
            throw PECI_EXCEPTION_ADDR(address, "getCpuId() failed");
        }

        return cpuId;
    }

    std::tuple<uint64_t, uint8_t> detectCores(uint8_t address, uint32_t cpuId)
    {
        uint32_t coreMaskLow;
        uint32_t coreMaskHigh;

        if (!peciAdapter->getCoreMaskLow(address, cpuId, coreMaskLow))
        {
            throw PECI_EXCEPTION_ADDR(address, "getCoreMaskLow() failed");
        }

        if (!peciAdapter->getCoreMaskHigh(address, cpuId, coreMaskHigh))
        {
            throw PECI_EXCEPTION_ADDR(address, "getCoreMaskHigh() failed");
        }

        uint64_t coreMask = coreMaskLow;
        coreMask |= static_cast<uint64_t>(coreMaskHigh) << 32U;
        uint64_t coreCount = static_cast<uint8_t>(
            static_cast<std::bitset<sizeof(coreMask) * 8U>>(coreMask).count());

        return {coreMask, coreCount};
    }

    uint32_t detectMaxNonTurboFreq(uint8_t address, uint32_t cpuId) const
    {
        uint8_t maxNonTurboRatio;

        if (!peciAdapter->getMaxNonTurboRatio(address, cpuId, maxNonTurboRatio))
        {
            throw PECI_EXCEPTION_ADDR(address, "getMaxNonTurboRatio() failed");
        }

        return maxNonTurboRatio * peci::abi::cpuFreqRatioMHz;
    }

    uint32_t detectMaxTurboFreq(uint8_t address, uint8_t coreCount) const
    {
        uint8_t maxTurboRatio;
        uint8_t tmpCoreCount = coreCount;
        uint8_t coreIdx;

        /**
         * Response of Get Turbo Ratio PECI command returns ratio for 4 cores is
         * such a way that when we have number of cores that is divisible by 4
         * we need to decrement the count.
         */
        if (tmpCoreCount % peci::abi::turboRatioCoreCount == 0)
        {
            tmpCoreCount--;
        }

        tmpCoreCount /= static_cast<uint8_t>(peci::abi::turboRatioCoreCount);
        coreIdx = static_cast<uint8_t>(
            (coreCount - 1U) - (peci::abi::turboRatioCoreCount * tmpCoreCount));

        if (!peciAdapter->getMaxTurboRatio(address, tmpCoreCount, coreIdx,
                                           maxTurboRatio))
        {
            throw PECI_EXCEPTION_ADDR(address, "getMaxTurboRatio() failed");
        }

        return maxTurboRatio * peci::abi::cpuFreqRatioMHz;
    }

    bool detectTurboValue(uint8_t address, uint32_t cpuId)
    {
        bool turbo;

        if (!peciAdapter->isTurboEnabled(address, cpuId, turbo))
        {
            throw PECI_EXCEPTION_ADDR(address, "isTurboEnabled() failed");
        }

        return turbo;
    }

    uint8_t detectBusNumber(uint8_t address, uint32_t cpuId)
    {
        uint8_t busNumber;

        if (!peciAdapter->getCpuBusNumber(address, cpuId, busNumber))
        {
            throw PECI_EXCEPTION_ADDR(address, "getCpuBusNumber() failed");
        }

        return busNumber;
    }
};

} // namespace metrics

} // namespace peci

} // namespace cups
