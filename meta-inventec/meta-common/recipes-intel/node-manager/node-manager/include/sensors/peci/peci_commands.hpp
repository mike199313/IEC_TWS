/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2022 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with
 * no express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#pragma once

#include "common_types.hpp"
#include "peci_types.hpp"
#include "utility/performance_monitor.hpp"

#include <optional>
#include <sstream>

#include "peci.h"

namespace nodemanager
{

class PeciCommandsIf
{
  public:
    virtual ~PeciCommandsIf() = default;

    virtual std::optional<uint64_t>
        getC0CounterSensor(const DeviceIndex cpuIndex) const = 0;
    virtual std::optional<uint64_t>
        getEpiCounterSensor(const DeviceIndex cpuIndex) const = 0;
    virtual std::optional<uint32_t>
        getCpuId(const DeviceIndex cpuIndex) const = 0;
    virtual std::optional<uint32_t>
        getCpuDieMask(const DeviceIndex cpuIndex) const = 0;
    virtual std::optional<uint32_t>
        isTurboEnabled(const DeviceIndex cpuIndex,
                       const uint32_t cpuId) const = 0;
    virtual std::optional<uint32_t>
        getCoreMaskLow(const DeviceIndex cpuIndex,
                       const uint32_t cpuId) const = 0;
    virtual std::optional<uint32_t>
        getCoreMaskHigh(const DeviceIndex cpuIndex,
                        const uint32_t cpuId) const = 0;
    virtual std::optional<uint8_t>
        getMaxNonTurboRatio(const DeviceIndex cpuIndex,
                            const uint32_t cpuId) const = 0;
    virtual std::optional<std::vector<uint8_t>>
        getTurboRatio(const DeviceIndex cpuIndex, const uint32_t cpuId,
                      uint8_t coreCount, uint8_t hiLowSelect) const = 0;
    virtual std::optional<uint8_t>
        detectMinTurboRatio(const DeviceIndex cpuIndex, const uint32_t cpuId,
                            uint8_t coreCount) const = 0;
    virtual std::optional<uint8_t>
        detectMaxTurboRatio(const DeviceIndex cpuIndex,
                            const uint32_t cpuId) const = 0;
    virtual std::optional<uint8_t> detectCores(const DeviceIndex cpuIndex,
                                               const uint32_t cpuId) const = 0;
    virtual std::optional<uint8_t>
        getTurboRatioLimit(const DeviceIndex cpuIndex) const = 0;
    virtual bool setTurboRatio(const DeviceIndex cpuIndex,
                               const uint8_t newRatioLimit) const = 0;
    virtual std::optional<uint8_t>
        getMinOperatingRatio(const DeviceIndex cpuIndex,
                             const uint32_t cpuId) const = 0;
    virtual std::optional<uint8_t>
        getMaxEfficiencyRatio(const DeviceIndex cpuIndex,
                              const uint32_t cpuId) const = 0;
    virtual bool setHwpmPreference(const DeviceIndex cpuIndex,
                                   uint32_t value) const = 0;
    virtual bool setHwpmPreferenceBias(const DeviceIndex cpuIndex,
                                       uint32_t value) const = 0;
    virtual bool setHwpmPreferenceOverride(const DeviceIndex cpuIndex,
                                           uint32_t value) const = 0;
    virtual std::optional<uint8_t>
        getProchotRatio(const DeviceIndex cpuIndex) const = 0;
    virtual bool setProchotRatio(const DeviceIndex cpuIndex,
                                 const uint8_t newProchotRatio) const = 0;
};
class PeciCommands : public PeciCommandsIf
{
  public:
    PeciCommands() = default;
    virtual ~PeciCommands() = default;

    virtual std::optional<uint8_t>
        getMaxEfficiencyRatio(const DeviceIndex cpuIndex,
                              const uint32_t cpuId) const
    {
        try
        {
            request::GetPlatformInfoHigh req{cpuId};
            const auto res = executePeciCmd<response::GetPlatformInfoHigh>(
                __func__, cpuIndex, req);
            if (res)
            {
                return res->max_efficiency_ratio;
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    virtual std::optional<uint8_t>
        getMinOperatingRatio(const DeviceIndex cpuIndex,
                             const uint32_t cpuId) const
    {
        try
        {
            request::GetPlatformInfoHigh req{cpuId};
            const auto res = executePeciCmd<response::GetPlatformInfoHigh>(
                __func__, cpuIndex, req);
            if (res)
            {
                return res->min_operating_ratio;
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    std::optional<uint8_t> getProchotRatio(const DeviceIndex cpuIndex) const
    {
        request::GetProchotRatio req;
        const auto res =
            executePeciCmd<response::GetProchotRatio>(__func__, cpuIndex, req);
        if (res)
        {
            return res->prochot_ratio;
        }
        return std::nullopt;
    }

    bool setProchotRatio(const DeviceIndex cpuIndex,
                         const uint8_t newProchotRatio) const
    {
        request::SetProchotRatio req;
        req.payload.prochot_ratio = newProchotRatio;
        const auto res =
            executePeciCmd<response::SetProchotRatio>(__func__, cpuIndex, req);
        if (res)
        {
            return true;
        }
        return false;
    }

    std::optional<uint8_t> getTurboRatioLimit(const DeviceIndex cpuIndex) const
    {
        request::GetTurboRatioLimit req;
        const auto res = executePeciCmd<response::GetTurboRatioLimit>(
            __func__, cpuIndex, req);
        if (res)
        {
            return res->ratioLimit;
        }
        return std::nullopt;
    }

    bool setTurboRatio(const DeviceIndex cpuIndex,
                       const uint8_t newRatioLimit) const
    {
        request::SetTurboRatioLimit req;
        req.payload.ratioLimit = newRatioLimit;
        const auto res = executePeciCmd<response::SetTurboRatioLimit>(
            __func__, cpuIndex, req);
        if (res)
        {
            return true;
        }
        return false;
    }

    std::optional<uint64_t> getC0CounterSensor(const DeviceIndex cpuIndex) const
    {
        request::GetCpuC0Counter req;
        const auto res =
            executePeciCmd<response::GetCpuC0Counter>(__func__, cpuIndex, req);
        if (res)
        {
            return res->c0Counter;
        }
        return std::nullopt;
    }

    std::optional<uint64_t>
        getEpiCounterSensor(const DeviceIndex cpuIndex) const
    {
        request::GetCpuEpiCounter req;
        const auto res =
            executePeciCmd<response::GetCpuEpiCounter>(__func__, cpuIndex, req);
        if (res)
        {
            return res->epiCounter;
        }
        return std::nullopt;
    }

    std::optional<uint32_t> getCpuId(const DeviceIndex cpuIndex) const
    {
        request::GetCpuId req;
        const auto res =
            executePeciCmd<response::GetCpuId>(__func__, cpuIndex, req);
        if (res)
        {
            return res->cpuId;
        }
        return std::nullopt;
    }

    std::optional<uint32_t> getCpuDieMask(const DeviceIndex cpuIndex) const
    {
        request::GetCpuDieMask req;
        const auto res =
            executePeciCmd<response::GetCpuDieMask>(__func__, cpuIndex, req);
        if (res)
        {
            return res->mask;
        }
        return std::nullopt;
    }

    std::optional<uint32_t> isTurboEnabled(const DeviceIndex cpuIndex,
                                           const uint32_t cpuId) const
    {
        try
        {
            constexpr DeviceIndex energyEfficientTurboBit = 26;
            request::GetCapabilityRegister req{cpuId};
            const auto res = executePeciCmd<response::GetCapabilityRegister>(
                __func__, cpuIndex, req);
            if (res)
            {
                std::bitset<sizeof(res->capabilities) * 8U> bits(
                    res->capabilities);
                return bits[energyEfficientTurboBit];
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    std::optional<uint32_t> getCoreMaskLow(const DeviceIndex cpuIndex,
                                           const uint32_t cpuId) const
    {
        try
        {
            request::GetCoreMaskLow req{cpuId};
            const auto res =
                executePeciCmd<response::GetCoreMask>(__func__, cpuIndex, req);
            if (res)
            {
                return res->coreMask;
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    std::optional<uint32_t> getCoreMaskHigh(const DeviceIndex cpuIndex,
                                            const uint32_t cpuId) const
    {
        try
        {
            request::GetCoreMaskHigh req{cpuId};
            const auto res =
                executePeciCmd<response::GetCoreMask>(__func__, cpuIndex, req);
            if (res)
            {
                return res->coreMask;
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    std::optional<uint8_t> getMaxNonTurboRatio(const DeviceIndex cpuIndex,
                                               const uint32_t cpuId) const
    {
        try
        {
            request::GetPlatformInfoLow req{cpuId};
            const auto res = executePeciCmd<response::GetPlatformInfoLow>(
                __func__, cpuIndex, req);
            if (res)
            {
                return res->max_non_turbo_ratio;
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    std::optional<std::vector<uint8_t>>
        getTurboRatio(const DeviceIndex cpuIndex, const uint32_t cpuId,
                      uint8_t coreCount = 0, uint8_t hiLowSelect = 0) const
    {
        try
        {
            request::GetTurboRatio req{cpuId};
            if (request::getCpuModel(cpuId) == request::CpuModelType::gnr)
            {
                req.payload.gnr.hiLowSelect =
                    static_cast<uint8_t>(hiLowSelect & 0x1);
            }
            else
            {
                req.payload.defaultCpu.coreCount =
                    static_cast<uint8_t>(coreCount & 0x7F);
            }
            const auto res = executePeciCmd<response::GetTurboRatio>(
                __func__, cpuIndex, req);
            if (res)
            {
                std::vector<uint8_t> turboRatio(std::begin(res->ratio),
                                                std::end(res->ratio));

                return turboRatio;
            }
        }
        catch (const std::runtime_error& e)
        {
            Logger::log<LogLevel::debug>(e.what());
        }
        return std::nullopt;
    }

    std::optional<uint8_t> detectMinTurboRatio(const DeviceIndex cpuIndex,
                                               const uint32_t cpuId,
                                               uint8_t coreCount) const
    {
        if (request::getCpuModel(cpuId) == request::CpuModelType::gnr)
        {
            uint8_t hiSelect = 1;
            uint8_t lowSelect = 0;
            const auto respTurboRatioHigh =
                getTurboRatio(cpuIndex, cpuId, coreCount, hiSelect);
            const auto respTurboRatioLow =
                getTurboRatio(cpuIndex, cpuId, coreCount, lowSelect);

            if (respTurboRatioHigh && respTurboRatioLow)
            {
                auto it = std::find_if_not(respTurboRatioHigh->rbegin(),
                                           respTurboRatioHigh->rend(),
                                           [](uint8_t v) { return v == 0; });
                if (it == respTurboRatioHigh->rend())
                {
                    it = std::find_if_not(respTurboRatioLow->rbegin(),
                                          respTurboRatioLow->rend(),
                                          [](uint8_t v) { return v == 0; });
                }
                if (it != respTurboRatioLow->rend())
                {
                    return *it;
                }
            }
        }
        else
        {
            uint8_t tmpCoreCount = coreCount;
            uint8_t coreIdx;

            /**
             * Response of Get Turbo Ratio PECI command returns ratio for 4
             * cores in such a way that when we have number of cores that is
             * divisible by 4 we need to decrement the count.
             */
            if (tmpCoreCount % turboRatioCoreCount == 0)
            {
                tmpCoreCount--;
            }

            tmpCoreCount /= turboRatioCoreCount;
            coreIdx = static_cast<uint8_t>(
                (coreCount - 1U) - (turboRatioCoreCount * tmpCoreCount));

            const auto res = getTurboRatio(cpuIndex, cpuId, tmpCoreCount);
            if (res)
            {
                return (*res)[coreIdx];
            }
        }
#ifdef ENABLE_PECI
        Logger::log<LogLevel::debug>(
            "detectMinTurboRatio failed, cpuIndex: %d, cpuId: %d",
            unsigned{cpuIndex}, cpuId);
#endif
        return std::nullopt;
    }

    std::optional<uint8_t> detectMaxTurboRatio(const DeviceIndex cpuIndex,
                                               const uint32_t cpuId) const
    {
        const auto res = getTurboRatio(cpuIndex, cpuId);

        if (res)
        {
            return (*res)[0];
        }
#ifdef ENABLE_PECI
        Logger::log<LogLevel::debug>(
            "detectMaxTurboRatio failed, cpuIndex: %d, cpuId: %d",
            unsigned{cpuIndex}, cpuId);
#endif
        return std::nullopt;
    }

    std::optional<uint8_t> detectCores(const DeviceIndex cpuIndex,
                                       const uint32_t cpuId) const
    {
        const auto coreMaskLow = getCoreMaskLow(cpuIndex, cpuId);
        const auto coreMaskHigh = getCoreMaskHigh(cpuIndex, cpuId);

        if (coreMaskLow && coreMaskHigh)
        {
            uint64_t coreMask = *coreMaskLow;
            coreMask |= static_cast<uint64_t>(*coreMaskHigh) << 32U;
            uint8_t coreCount = static_cast<uint8_t>(
                static_cast<std::bitset<sizeof(coreMask) * 8U>>(coreMask)
                    .count());
            return coreCount;
        }
#ifdef ENABLE_PECI
        Logger::log<LogLevel::debug>(
            "detectCores failed, cpuIndex: %d, cpuId: %d", unsigned{cpuIndex},
            cpuId);
#endif
        return std::nullopt;
    }

    bool setHwpmPreference(const DeviceIndex cpuIndex, uint32_t value) const
    {
        request::SetHwpmPreferencePkgConfig req;
        req.data = value;
        const auto res = executePeciCmd<response::SetHwpmPreference>(
            __func__, cpuIndex, req);
        if (res)
        {
            return true;
        }
        return false;
    }

    bool setHwpmPreferenceBias(const DeviceIndex cpuIndex, uint32_t value) const
    {
        request::SetHwpmPreferenceBiasPkgConfig req;
        req.data = value;
        const auto res = executePeciCmd<response::SetHwpmPreferenceBias>(
            __func__, cpuIndex, req);
        if (res)
        {
            return true;
        }
        return false;
    }

    bool setHwpmPreferenceOverride(const DeviceIndex cpuIndex,
                                   uint32_t value) const
    {
        request::SetHwpmPreferenceOverridePkgConfig req;
        req.data = value;
        const auto res = executePeciCmd<response::SetHwpmPreferenceOverride>(
            __func__, cpuIndex, req);
        if (res)
        {
            return true;
        }
        return false;
    }

  private:
    inline uint8_t getPeciCPUAddress(const DeviceIndex cpuIndex) const
    {
        if constexpr (std::is_same<DeviceIndex, uint8_t>::value)
        {
            return PECI_TRANSPORT_CPU0_ADDRESS + cpuIndex;
        }
        else
        {
            throw std::logic_error("Cannot map DeviceIndex");
        }
    }

    template <class Response, class Request>
    std::optional<Response> executePeciCmd(const char* funName,
                                           const DeviceIndex cpuIndex,
                                           Request& req) const
    {
        Response rsp{};
        const EPECIStatus ret =
            executePeciCmd(getPeciCPUAddress(cpuIndex), req, rsp);
        if (ret == PECI_CC_SUCCESS && rsp.compCode == COMPLETION_CODE_SUCCESS)
        {
            return rsp;
        }
#ifdef ENABLE_PECI
        Logger::log<LogLevel::debug>(
            "%s failed, cpuIndex: %d, EPECIStatus: %d, "
            "Rsp.compCode: %d",
            funName, unsigned{cpuIndex},
            static_cast<std::underlying_type_t<EPECIStatus>>(ret),
            static_cast<unsigned>(rsp.compCode));
#endif
        return std::nullopt;
    }

    template <typename Req, typename Rsp>
    EPECIStatus executePeciCmd(uint8_t target, Req& req, Rsp& rsp) const
    {
#ifdef ENABLE_PECI
        const auto pReq = reinterpret_cast<uint8_t*>(&req);
        const auto pRsp = reinterpret_cast<uint8_t*>(&rsp);

        return peci_raw(target, static_cast<uint8_t>(sizeof(Rsp)), pReq,
                        static_cast<uint8_t>(sizeof(Req)), pRsp,
                        static_cast<uint8_t>(sizeof(Rsp)));
#else  // ENABLE_PECI
        return EPECIStatus::PECI_CC_TIMEOUT;
#endif // ENABLE_PECI
    }
};

} // namespace nodemanager
