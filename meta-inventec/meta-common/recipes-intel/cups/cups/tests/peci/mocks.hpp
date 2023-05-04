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

#include "peci/abi.hpp"
#include "peci/metrics/impl/core.hpp"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

struct Adapter
{
    class Mock
    {
      public:
        MOCK_METHOD2(getCpuId, bool(uint8_t, uint32_t&));
        MOCK_METHOD3(getCoreMaskLow, bool(uint8_t, uint32_t, uint32_t&));
        MOCK_METHOD3(getCoreMaskHigh, bool(uint8_t, uint32_t, uint32_t&));
        MOCK_METHOD2(getCpuC0Counter, bool(uint8_t, uint64_t&));
        MOCK_METHOD2(getMaxNonTurboRatio, bool(uint8_t, uint8_t&));
        MOCK_METHOD4(getMaxTurboRatio,
                     bool(uint8_t, uint8_t, uint8_t, uint8_t&));
        MOCK_METHOD2(isTurboEnabled, bool(uint8_t, bool&));
        MOCK_METHOD3(getCpuBusNumber, bool(uint8_t, uint32_t, uint8_t&));
        MOCK_METHOD7(isDIMMPopulated, bool(uint8_t, uint32_t, uint8_t, uint8_t,
                                           uint8_t, uint32_t, bool&));
        MOCK_METHOD2(getMemoryFreq, bool(uint8_t, uint32_t&));
        MOCK_METHOD7(getLinkStatus, bool(uint8_t, uint8_t, uint8_t, uint16_t,
                                         uint8_t&, uint8_t&, bool&));
        MOCK_METHOD4(getXppMr, bool(uint8_t, uint8_t, uint8_t, uint32_t&));
        MOCK_METHOD4(getXppMer, bool(uint8_t, uint8_t, uint8_t, uint32_t&));
        MOCK_METHOD4(getXppErConf, bool(uint8_t, uint8_t, uint8_t, uint32_t&));
        MOCK_METHOD4(setXppMr, bool(uint8_t, uint8_t, uint8_t, uint32_t));
        MOCK_METHOD4(setXppMer, bool(uint8_t, uint8_t, uint8_t, uint32_t));
        MOCK_METHOD4(setXppErConf, bool(uint8_t, uint8_t, uint8_t, uint32_t));

        virtual ~Mock()
        {}
    };

    static std::unique_ptr<Mock> mock;

    static bool getCpuC0Counter(uint8_t target, uint64_t& c0Counter)
    {
        return mock->getCpuC0Counter(target, c0Counter);
    }

    static bool getCpuId(uint8_t target, uint32_t& cpuId)
    {
        return mock->getCpuId(target, cpuId);
    }

    static bool getCoreMaskLow(uint8_t target, uint32_t cpuId,
                               uint32_t& coreMaskLow)
    {
        return mock->getCoreMaskLow(target, cpuId, coreMaskLow);
    }

    static bool getCoreMaskHigh(uint8_t target, uint32_t cpuId,
                                uint32_t& coreMaskHigh)
    {
        return mock->getCoreMaskHigh(target, cpuId, coreMaskHigh);
    }

    static bool getMaxNonTurboRatio(uint8_t target, uint8_t& maxNonTurboRatio)
    {
        return mock->getMaxNonTurboRatio(target, maxNonTurboRatio);
    }

    static bool getMaxTurboRatio(uint8_t target, uint8_t coreCount,
                                 uint8_t coreIdx, uint8_t& maxTurboRatio)
    {
        return mock->getMaxTurboRatio(target, coreCount, coreIdx,
                                      maxTurboRatio);
    }

    static bool isTurboEnabled(uint8_t target, bool& turbo)
    {
        return mock->isTurboEnabled(target, turbo);
    }

    static bool getCpuBusNumber(uint8_t target, uint32_t cpuId,
                                uint8_t& busNumber)
    {
        return mock->getCpuBusNumber(target, cpuId, busNumber);
    }

    static bool isDIMMPopulated(uint8_t cpuAddress, uint32_t cpuId,
                                uint8_t cpuBusNumber, uint8_t dev, uint8_t func,
                                uint32_t reg, bool& dimmPopulated)
    {
        return mock->isDIMMPopulated(cpuAddress, cpuId, cpuBusNumber, dev, func,
                                     reg, dimmPopulated);
    }

    static bool getMemoryFreq(uint8_t cpuAddress, uint32_t& frequency)
    {
        return mock->getMemoryFreq(cpuAddress, frequency);
    }

    static bool getLinkStatus(uint8_t target, uint8_t bus, uint8_t dev,
                              uint16_t reg, uint8_t& speed, uint8_t& width,
                              bool& active)
    {
        return mock->getLinkStatus(target, bus, dev, reg, speed, width, active);
    }

    static bool getXppMr(uint8_t target, uint8_t bus, uint8_t dev,
                         uint32_t& xppMr)
    {
        return mock->getXppMr(target, bus, dev, xppMr);
    }

    static bool getXppMer(uint8_t target, uint8_t bus, uint8_t dev,
                          uint32_t& xppMer)
    {
        return mock->getXppMer(target, bus, dev, xppMer);
    }

    static bool getXppErConf(uint8_t target, uint8_t bus, uint8_t dev,
                             uint32_t& xppErConf)
    {
        return mock->getXppErConf(target, bus, dev, xppErConf);
    }

    static bool setXppMr(uint8_t target, uint8_t bus, uint8_t dev,
                         uint32_t xppMr)
    {
        return mock->setXppMr(target, bus, dev, xppMr);
    }

    static bool setXppMer(uint8_t target, uint8_t bus, uint8_t dev,
                          uint32_t xppMer)
    {
        return mock->setXppMer(target, bus, dev, xppMer);
    }

    static bool setXppErConf(uint8_t target, uint8_t bus, uint8_t dev,
                             uint32_t xppErConf)
    {
        return mock->setXppErConf(target, bus, dev, xppErConf);
    }
};

struct Driver
{
    class Mock
    {
      public:
        MOCK_METHOD7(commandHandler,
                     bool(const uint8_t, const uint8_t*, const size_t, uint8_t*,
                          const size_t, const bool, const std::string&));

        virtual ~Mock()
        {}
    };

    static std::unique_ptr<Mock> mock;

    static bool commandHandler(const uint8_t target, const uint8_t* pReq,
                               const size_t reqSize, uint8_t* pRsp,
                               const size_t rspSize, const bool logError = true,
                               const std::string& name = "")
    {
        return mock->commandHandler(target, pReq, reqSize, pRsp, rspSize,
                                    logError, name);
    }
};

struct Impl
{
    class Mock
    {
      public:
        MOCK_METHOD0(getAddress, uint8_t());
        MOCK_METHOD0(getMaxUtil, uint64_t());
        MOCK_METHOD0(probe, std::optional<uint64_t>());

        virtual ~Mock()
        {}
    };

    static std::unique_ptr<Mock> mock;

    static uint8_t getAddress()
    {
        return mock->getAddress();
    }

    static uint64_t getMaxUtil()
    {
        return mock->getMaxUtil();
    }

    static std::optional<uint64_t> probe()
    {
        return mock->probe();
    }
};
