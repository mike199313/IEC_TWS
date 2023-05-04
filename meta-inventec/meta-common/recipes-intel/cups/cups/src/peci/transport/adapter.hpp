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
#include "utils/traits.hpp"

#include <boost/crc.hpp>

#include <cstdint>
#include <string>

namespace cups
{

namespace peci
{

namespace transport
{

static inline uint8_t calculateAwFcs(uint8_t target, uint8_t reqLen,
                                     uint8_t rspLen, const uint8_t* reqBuf)
{
    // AW FCS - Assured Write FCS
    // Control sum required in certain write operations, see PECI spec for
    // more information

    // 0x07 represents expected polynomial: x^8 + x^2 + x^1 + x^0
    boost::crc_optimal<8, 0x07, 0, 0, false, false> crc_8;
    crc_8(target);
    crc_8(reqLen);
    crc_8(rspLen);
    crc_8 = std::for_each(reqBuf, reqBuf + reqLen - 1, crc_8);
    // Flip MSb, per PECI specification
    return (crc_8() ^ 0x80);
}

bool commandHandler(const uint8_t target, const uint8_t* pReq,
                    const size_t reqSize, uint8_t* pRsp, const size_t rspSize,
                    const bool logError = true, const std::string& name = "");

class Adapter
{
  public:
    template <typename Req, typename Rsp>
    bool executePeciCommand(uint8_t target, Req& req, Rsp& rsp,
                            bool logError = true) const
    {
        const auto pReq = reinterpret_cast<uint8_t*>(&req);
        const auto pRsp = reinterpret_cast<uint8_t*>(&rsp);
        auto reqName = utils::typeName<Req>();

        return commandHandler(target, pReq, sizeof(Req), pRsp, sizeof(Rsp),
                              logError, reqName);
    }

    bool getCpuId(uint8_t target, uint32_t& cpuId) const
    {
        abi::request::GetCpuId req;
        abi::response::GetCpuId rsp;
        constexpr auto doNotPrintErrors = false;

        // This command is used continuously to detect new CPUs - we don't want
        // to pollute log with errors about this completely normal situation
        // (CPU socket is unpopulated)
        if (!executePeciCommand(target, req, rsp, doNotPrintErrors))
        {
            return false;
        }

        cpuId = rsp.cpuId;
        return true;
    }

    bool getCpuC0Counter(uint8_t target, uint64_t& c0Counter) const
    {
        abi::request::GetCpuC0Counter req;
        abi::response::GetCpuC0Counter rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        c0Counter = rsp.c0Counter;
        return true;
    }

    bool getCoreMaskLow(uint8_t target, uint32_t cpuId,
                        uint32_t& coreMaskLow) const
    {
        abi::request::GetCoreMaskLow req(cpuId);
        abi::response::GetCoreMask rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        coreMaskLow = rsp.coreMask;
        return true;
    }

    bool getCoreMaskHigh(uint8_t target, uint32_t cpuId,
                         uint32_t& coreMaskHigh) const
    {
        abi::request::GetCoreMaskHigh req(cpuId);
        abi::response::GetCoreMask rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        coreMaskHigh = rsp.coreMask;
        return true;
    }

    bool getMaxNonTurboRatio(uint8_t target, uint32_t cpuId,
                             uint8_t& maxNonTurboRatio) const
    {
        abi::request::GetMaxNonTurboRatio req(cpuId);
        abi::response::GetMaxNonTurboRatio rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        maxNonTurboRatio = rsp.ratio;
        return true;
    }

    bool getMaxTurboRatio(uint8_t target, uint8_t coreCount, uint8_t coreIdx,
                          uint8_t& maxTurboRatio) const
    {
        abi::request::GetMaxTurboRatio req;
        abi::response::GetMaxTurboRatio rsp;

        req.payload.coreCount = static_cast<uint8_t>(coreCount & 0x7F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        maxTurboRatio = rsp.ratio[coreIdx];
        return true;
    }

    bool isTurboEnabled(uint8_t target, uint32_t cpuId, bool& turbo) const
    {
        abi::request::GetCapabilityRegister req(cpuId);
        abi::response::GetCapabilityRegister rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        turbo = static_cast<bool>(rsp.energyEfficientTurbo);
        return true;
    }

    bool getCpuBusNumber(uint8_t target, uint32_t cpuId,
                         uint8_t& busNumber) const
    {
        abi::request::GetCpuBusNumber req(cpuId);
        abi::response::GetCpuBusNumber rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        busNumber = rsp.busNumber0;
        return true;
    }

    bool isDIMMPopulated(uint8_t target, uint32_t cpuId, uint8_t bus,
                         uint8_t dev, uint8_t func, uint32_t reg,
                         bool& dimmPopulated) const
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
            case cpu::model::gnr:
                return isDIMMPopulated<abi::request::GetDimmmtr64>(
                    target, bus, dev, func, reg, dimmPopulated);

            default:
                throw PECI_EXCEPTION_ADDR(target, "Unhandled");
        }
    }

    template <typename Request>
    bool isDIMMPopulated(uint8_t target, uint8_t bus, uint8_t dev, uint8_t func,
                         uint32_t reg, bool& dimmPopulated) const
    {
        Request req;
        abi::response::GetDimmmtr rsp;

        req.payload.bus = bus;
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);
        req.payload.func = static_cast<uint8_t>(func & 7);
        req.payload.reg = reg;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        dimmPopulated = static_cast<bool>(rsp.dimmPop);
        return true;
    }

    bool getMemoryFreq(uint8_t target, uint32_t cpuId, uint32_t& freq) const
    {
        abi::request::GetMemoryFreq req(cpuId);
        abi::response::GetMemoryFreq rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        freq = rsp.frequency;

        if (rsp.type == 0)
        {
            freq *= peci::abi::memory::freqRatio133Mhz;
        }
        else if (rsp.type == 1)
        {
            freq *= peci::abi::memory::freqRatio100Mhz;
        }
        else
        {
            LOG_ERROR << "Unexpected memory frequency type: " << rsp.type;
            return false;
        }
        freq /= peci::abi::memory::freqRatioDiv;

        return true;
    }

    bool getMemoryRdCounter(uint8_t target, uint32_t cpuId, uint8_t bus,
                            uint8_t dev, uint64_t& counter) const
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                return getMemoryRdCounter<abi::request::GetMemoryRdCounter64>(
                    target, bus, dev, counter);

            default:
                throw PECI_EXCEPTION_ADDR(target, "Unhandled");
        }
    }

    template <typename Request>
    bool getMemoryRdCounter(uint8_t target, uint8_t bus, uint8_t dev,
                            uint64_t& counter) const
    {
        Request req;
        abi::response::GetMemoryCounter rsp;

        req.payload.bus = bus;
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        counter = rsp.counter64bytes;
        return true;
    }

    bool getMemoryWrCounter(uint8_t target, uint32_t cpuId, uint8_t bus,
                            uint8_t dev, uint64_t& counter) const
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                return getMemoryWrCounter<abi::request::GetMemoryWrCounter64>(
                    target, bus, dev, counter);

            default:
                throw PECI_EXCEPTION_ADDR(target, "Unhandled");
        }
    }

    template <typename Request>
    bool getMemoryWrCounter(uint8_t target, uint8_t bus, uint8_t dev,
                            uint64_t& counter) const
    {
        Request req;
        abi::response::GetMemoryCounter rsp;

        req.payload.bus = bus;
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        counter = rsp.counter64bytes;
        return true;
    }

    bool getMemoryRwCounters(uint8_t target, uint16_t aggregatorIdx,
                             uint16_t sampleIdx, uint32_t& counterWr,
                             uint32_t& counterRd) const
    {
        abi::request::GetMemoryRwCounter req(aggregatorIdx, sampleIdx);
        abi::response::GetMemoryRwCounter rsp;

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        counterWr = rsp.cacheLineWrites;
        counterRd = rsp.cacheLineReads;
        return true;
    }

    bool getLinkStatus(uint8_t target, uint8_t bus, uint8_t dev, uint16_t reg,
                       uint8_t& speed, uint8_t& width, bool& active) const
    {
        abi::request::GetLinkStatus req;
        abi::response::GetLinkStatus rsp;
        constexpr auto doNotPrintErrors = false;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);
        req.payload.reg = static_cast<uint16_t>(reg & 0xFFF);

        // Some of the PCI links can be disabled and error should not be logged
        // in normal circuimstances
        if (!executePeciCommand(target, req, rsp, doNotPrintErrors))
        {
            return false;
        }

        speed = rsp.speed;
        width = rsp.width;
        active = static_cast<bool>(rsp.active);
        return true;
    }

    bool getXppMr(uint8_t target, uint8_t bus, uint8_t dev,
                  uint32_t& xppMr) const
    {
        abi::request::GetXPPMR req;
        abi::response::GetXPPMR rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        xppMr = rsp.reg.raw;
        return true;
    }

    bool getXppMer(uint8_t target, uint8_t bus, uint8_t dev,
                   uint32_t& xppMer) const
    {
        abi::request::GetXPPMER req;
        abi::response::GetXPPMER rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        xppMer = rsp.reg.raw;
        return true;
    }

    bool getXppErConf(uint8_t target, uint8_t bus, uint8_t dev,
                      uint32_t& xppErConf) const
    {
        abi::request::GetXPPERConf req;
        abi::response::GetXPPERConf rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        xppErConf = rsp.reg.raw;
        return true;
    }

    bool getXppMdl(uint8_t target, uint8_t bus, uint8_t dev,
                   uint32_t& counter) const
    {
        abi::request::GetXPPMdl req;
        abi::response::GetXPPMdl rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        counter = rsp.counterDwords;
        return true;
    }

    bool getXppMdh(uint8_t target, uint8_t bus, uint8_t dev,
                   uint32_t& counter) const
    {
        abi::request::GetXPPMdh req;
        abi::response::GetXPPMdh rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        counter = rsp.counterDwords0;
        return true;
    }

    bool getDlu(uint8_t target, uint8_t bus, uint64_t& counter) const
    {
        abi::request::GetDlu req;
        abi::response::GetDlu rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        // DMI port (PCIe gen3) has only one active link connected to PCH
        // and only one corresponding LNKSTS register, hence we are
        // collecting data only from counter related with that link
        counter = rsp.counterGigabytes[0];
        return true;
    }

    bool setXppMr(uint8_t target, uint8_t bus, uint8_t dev,
                  uint32_t xppMr) const
    {
        abi::request::SetXPPMR req;
        abi::response::SetXPPRegister rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);
        req.reg.raw = xppMr;
        req.checksum = calculateAwFcs(target, sizeof(req), sizeof(rsp),
                                      reinterpret_cast<uint8_t*>(&req));

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        return true;
    }

    bool setXppMer(uint8_t target, uint8_t bus, uint8_t dev,
                   uint32_t xppMer) const
    {
        abi::request::SetXPPMER req;
        abi::response::SetXPPRegister rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);
        req.reg.raw = xppMer;
        req.checksum = calculateAwFcs(target, sizeof(req), sizeof(rsp),
                                      reinterpret_cast<uint8_t*>(&req));

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        return true;
    }

    bool setXppErConf(uint8_t target, uint8_t bus, uint8_t dev,
                      uint32_t xppErConf) const
    {
        abi::request::SetXPPERConf req;
        abi::response::SetXPPRegister rsp;

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);
        req.reg.raw = xppErConf;
        req.checksum = calculateAwFcs(target, sizeof(req), sizeof(rsp),
                                      reinterpret_cast<uint8_t*>(&req));

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        return true;
    }

    bool getXppMonFrCtrClk(uint8_t target, uint8_t bus, uint8_t dev,
                           uint64_t& xppMonFrCtrClk) const
    {
        abi::request::GetXPPMonFrCtrClk req{};
        abi::response::GetXPPMonFrCtr rsp{};

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        xppMonFrCtrClk = rsp.count;
        return true;
    }

    bool getXppMonFrCtr(uint8_t target, uint8_t bus, uint8_t dev,
                        uint16_t offset, uint64_t& xppMonFrCtrClk) const
    {
        abi::request::GetXPPMonFrCtr req{};
        abi::response::GetXPPMonFrCtr rsp{};

        req.payload.bus = static_cast<uint8_t>(bus & 0xF);
        req.payload.dev = static_cast<uint8_t>(dev & 0x1F);
        req.payload.reg =
            static_cast<uint16_t>((req.payload.reg + offset) & 0xFFF);

        if (!executePeciCommand(target, req, rsp))
        {
            return false;
        }

        xppMonFrCtrClk = rsp.count;
        return true;
    }
};

} // namespace transport

} // namespace peci

} // namespace cups
