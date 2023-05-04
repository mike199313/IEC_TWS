/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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

#include "config/config.hpp"
#include "config/config_values.hpp"
#include "loggers/log.hpp"
#include "utility/ipmb.hpp"

#include <chrono>

namespace nodemanager
{
using namespace std::literals::chrono_literals;

static constexpr const uint8_t kDisablingSpsNmRetryMax = 3;
static constexpr const auto kRetryDuration = 25ms;

/**
 * @brief Class that determines which NM should work either OpenBMC or SPS.
 * Depending on configuration settings this class is able to disable the SPS NM.
 */
class SpsIntegrator
{
  public:
    SpsIntegrator(const SpsIntegrator&) = delete;
    SpsIntegrator& operator=(const SpsIntegrator&) = delete;
    SpsIntegrator(SpsIntegrator&&) = delete;
    SpsIntegrator& operator=(SpsIntegrator&&) = delete;
    SpsIntegrator(std::shared_ptr<sdbusplus::asio::connection> busArg) :
        bus(std::move(busArg))
    {
    }

    /**
     * @brief Depending on the InitializationMode read from the configuration
     * properly on/off the SPS NM.
     *
     * @return true in case when this OpenBMC NM should continue to run.
     * @return false in case when this OpenBMC NM should be disabled
     * immediately.
     */
    bool shouldNmStart()
    {
        const auto cfgInitMode =
            Config::getInstance().getGeneralPresets().nmInitializationMode;

        InitializationMode initMode{InitializationMode::warningStopBmcNm};
        if (isEnumCastSafe<InitializationMode>(kInitializationModeSet,
                                               cfgInitMode))
        {
            initMode = static_cast<InitializationMode>(cfgInitMode);
        }
        else
        {
            Logger::log<LogLevel::warning>(
                "Unknown InitializationMode: %d, using default value: 0",
                static_cast<unsigned>(cfgInitMode));
        }
        Logger::log<LogLevel::debug>("InitializationMode: %d",
                                     static_cast<unsigned>(initMode));

        switch (initMode)
        {
            case InitializationMode::warningStopBmcNm:
                return warningStopBmcNm();
            case InitializationMode::criticalStopBmcNm:
                return criticalStopBmcNm();
            case InitializationMode::warningDisableSpsNm:
                return warningDisableSpsNm();
            case InitializationMode::stopBmcNmUnconditionally:
                return stopBmcNmUnconditionally();
            default:
                Logger::log<LogLevel::error>("initializationMode out of range, "
                                             "assuming default flow mode:0");
                return warningStopBmcNm();
        }
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;

    bool warningStopBmcNm()
    {
        if (isSpsNmEnabled())
        {
            RedfishLogger::logStoppingNm();
            Logger::log<LogLevel::warning>(
                "SPS NM enabled, stopping OpenBMC NM");
            return false;
        }
        Logger::log<LogLevel::info>("SPS NM disabled, starting OpenBMC NM");
        return true;
    }

    bool criticalStopBmcNm()
    {
        if (isSpsNmEnabled())
        {
            RedfishLogger::logStoppingNm();
            Logger::log<LogLevel::warning>(
                "SPS NM enabled, stopping OpenBMC NM");
            return false;
        }
        Logger::log<LogLevel::info>("SPS NM disabled, starting OpenBMC NM");
        return true;
    }

    bool warningDisableSpsNm()
    {
        request::GetCapabilities req{};
        std::optional<response::GetCapabilities> res =
            IpmbUtil::ipmbSendRequest<response::GetCapabilities>(
                bus, req, kIpmiNetFnOem, kIpmiGetNmCapabilitiesCmd);

        if (res && res->assistModule.nm == kSupportedAndEnabledValue)
        {
            Logger::log<LogLevel::info>("SPS NM enabled, disabling...");
            if (tryDisableSpsNm(res->assistModule))
            {
                if (tryColdResetSPS())
                {
                    Logger::log<LogLevel::info>(
                        "SPS NM disabled, starting OpenBMC NM");
                    return true;
                }
            }
            RedfishLogger::logUnableToDisableSpsNm();
            Logger::log<LogLevel::error>(
                "Unable to disable the SPS NM, stopping OpenBMC NM");
            return false;
        }
        Logger::log<LogLevel::info>("SPS NM disabled, starting OpenBMC NM");
        return true;
    }

    bool stopBmcNmUnconditionally()
    {
        RedfishLogger::logInitializationMode3();
        Logger::log<LogLevel::warning>(
            "InitializationMode: 3, stopping OpenBMC NM unconditionally");
        return false;
    }

    bool isSpsNmEnabled()
    {
        request::GetCapabilities req{};
        std::optional<response::GetCapabilities> res =
            IpmbUtil::ipmbSendRequest<response::GetCapabilities>(
                bus, req, kIpmiNetFnOem, kIpmiGetNmCapabilitiesCmd);
        if (res)
        {
            return kSupportedAndEnabledValue == res->assistModule.nm;
        }
        Logger::log<LogLevel::warning>(
            "No positive reposnse from SPS NM, assuming it is disabled");
        return false;
    }

    bool tryDisableSpsNm(const AssistModuleCapabilities& assist)
    {
        request::SetCapabilities setReq{};
        setReq.assistModule = assist;
        correctAssistModule(setReq.assistModule);
        setReq.assistModule.nm = kSupportedAndDisabledValue;

        for (uint8_t retry = 0; retry <= kDisablingSpsNmRetryMax; retry++)
        {
            if (IpmbUtil::ipmbSendRequest<response::SetCapabilities>(
                    bus, setReq, kIpmiNetFnOem, kIpmiSetNmCapabilitiesCmd))
            {
                return true;
            }
            Logger::log<LogLevel::error>("Cannot disable the SPS NM, retry: %d",
                                         unsigned{retry});
            std::this_thread::sleep_for(kRetryDuration);
        }
        return false;
    }

    bool tryColdResetSPS()
    {
        for (uint8_t retry = 0; retry <= kDisablingSpsNmRetryMax; retry++)
        {
            if (IpmbUtil::ipmbSendColdRequest(bus))
            {
                return true;
            }
            Logger::log<LogLevel::error>("Cannot ColdReset the SPS, retry: %d",
                                         unsigned{retry});
            std::this_thread::sleep_for(kRetryDuration);
        }
        return false;
    }

    /**
     * @brief Value 0 is not allowed when the AssistModuleCapabilities is used
     * in ipmi command `SetCapabilites`. This function verifies if these values
     * are used and sets proper acceptable values.
     *
     * @param assist
     */
    void correctAssistModule(AssistModuleCapabilities& assist)
    {
        if (assist.bios == 0)
        {
            assist.bios = kSupportedAndDisabledValue;
        }
        if (assist.ras == 0)
        {
            assist.ras = kSupportedAndDisabledValue;
        }
        if (assist.performance == 0)
        {
            assist.performance = kSupportedAndDisabledValue;
        }
    }
};

} // namespace nodemanager