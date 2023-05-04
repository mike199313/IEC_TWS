/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
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
#include "loggers/log.hpp"
#include "policy_enums.hpp"

#include <boost/asio.hpp>
#include <sdbusplus/asio/property.hpp>

namespace nodemanager
{

using LimitExceptionActionCallback =
    std::function<void(const boost::system::errc::errc_t)>;

class LimitExceptionHandlerIf
{
  public:
    virtual ~LimitExceptionHandlerIf() = default;
    virtual void doAction(LimitException,
                          const LimitExceptionActionCallback& = nullptr) = 0;
};

struct LimitExceptionHandleDbusConfig
{
    std::chrono::seconds softShutdownTimeout{30};
    std::chrono::seconds powerDownTimeout{15};
    std::string hostStateServiceName{"xyz.openbmc_project.State.Host"};
    std::string chassisStateServiceName{"xyz.openbmc_project.State.Chassis"};
    std::string hostStateObjectPath{"/xyz/openbmc_project/state/host0"};
    std::string chassisStateObjectPath{"/xyz/openbmc_project/state/chassis0"};
    static constexpr const auto kHostStateServiceInterface{
        "xyz.openbmc_project.State.Host"};
    static constexpr const auto kChassisStateServiceInterface{
        "xyz.openbmc_project.State.Chassis"};
} kLimitExceptionDbusConfig;

class LimitExceptionHandler
    : public LimitExceptionHandlerIf,
      public std::enable_shared_from_this<LimitExceptionHandler>
{
  public:
    LimitExceptionHandler() = delete;
    LimitExceptionHandler(const LimitExceptionHandler&) = delete;
    LimitExceptionHandler& operator=(const LimitExceptionHandler&) = delete;
    LimitExceptionHandler(LimitExceptionHandler&&) = delete;
    LimitExceptionHandler& operator=(LimitExceptionHandler&&) = delete;

    LimitExceptionHandler(const std::string& policyPathArg,
                          const LimitExceptionHandleDbusConfig configArg,
                          std::shared_ptr<sdbusplus::asio::connection> busArg) :
        policyPath(policyPathArg),
        config(configArg), bus(busArg)
    {
    }

    virtual ~LimitExceptionHandler() = default;

    void doAction(LimitException actionType,
                  const LimitExceptionActionCallback& completionCallback =
                      nullptr) override
    {
        switch (actionType)
        {
            case LimitException::noAction:
                break;
            case LimitException::powerOff:
                actionPowerOff(completionCallback);
                break;
            case LimitException::logEvent:
                RedfishLogger::logLimitExceptionOccurred(policyPath);
                Logger::log<LogLevel::error>("Limit Exception occured");
                break;
            case LimitException::logEventAndPowerOff:
                RedfishLogger::logLimitExceptionOccurred(policyPath);
                Logger::log<LogLevel::error>("Limit Exception occured");
                actionPowerOff(completionCallback);
                break;
            default:
                Logger::log<LogLevel::error>(
                    "Unknown LimitException action type");
                break;
        }
    }

  private:
    struct PowerOffContext
    {
        PowerOffContext() = delete;
        PowerOffContext(boost::asio::io_context& ctx,
                        const LimitExceptionActionCallback& callback) :
            softShutdownTimer(ctx),
            powerDownTimer(ctx), completionCallback(callback)
        {
        }
        boost::asio::steady_timer softShutdownTimer;
        boost::asio::steady_timer powerDownTimer;
        bool softShutdownTimeoutOccured{false};
        bool powerDownTimeoutOccured{false};
        LimitExceptionActionCallback completionCallback{nullptr};
    };
    const std::string& policyPath;
    LimitExceptionHandleDbusConfig config;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    LimitException action{LimitException::noAction};

    void actionPowerOff(const LimitExceptionActionCallback& completionCallback)
    {
        RedfishLogger::logInitializeSoftShutdown();
        Logger::log<LogLevel::info>(
            "NodeManager is about to initiate soft shutdown within next 30s");
        auto ctx = std::make_shared<PowerOffContext>(bus->get_io_context(),
                                                     completionCallback);
        startSoftShutdownTimer(ctx);
        initSoftShutdown(handleHostStateChangeError(ctx),
                         handleHostStateChangeSuccess(ctx));
    }

    inline void startSoftShutdownTimer(std::shared_ptr<PowerOffContext> ctx)
    {
        ctx->softShutdownTimer.expires_after(config.softShutdownTimeout);
        ctx->softShutdownTimer.async_wait(
            [self = shared_from_this(), ctx](boost::system::error_code ec) {
                if (ec)
                {
                    return;
                }
                ctx->softShutdownTimeoutOccured = true;
                self->powerDownProcedure(ctx);
            });
    }

    std::function<void(void)>
        handleHostStateChangeSuccess(std::shared_ptr<PowerOffContext> ctx)
    {
        return [self = shared_from_this(), ctx]() {
            if (!ctx->softShutdownTimeoutOccured)
            {
                self->getPlatformPowerState(
                    [self, ctx](boost::system::error_code ec,
                                std::string powerState) {
                        if (ec)
                        {
                            Logger::log<LogLevel::error>(
                                "Err %lu: DBus 'GetProperty' CurrentPowerState "
                                "call failed",
                                ec.value());
                            return;
                        }
                        if (powerState ==
                            "xyz.openbmc_project.State.Chassis.PowerState.Off")
                        {
                            ctx->softShutdownTimer.cancel();
                            if (ctx->completionCallback)
                            {
                                ctx->completionCallback(
                                    boost::system::errc::errc_t::success);
                            }
                            return;
                        }
                        else
                        {
                            self->handleHostStateChangeSuccess(ctx);
                        }
                    });
            }
        };
    }

    std::function<void(boost::system::error_code)>
        handleHostStateChangeError(std::shared_ptr<PowerOffContext> ctx)
    {
        return [self = shared_from_this(), ctx](boost::system::error_code ec) {
            if (ec == boost::system::errc::not_supported)
            {
                ctx->softShutdownTimer.cancel();
                self->powerDownProcedure(ctx);
            }
            else
            {
                if (!ctx->softShutdownTimeoutOccured)
                {
                    self->initSoftShutdown(
                        self->handleHostStateChangeError(ctx),
                        self->handleHostStateChangeSuccess(ctx));
                }
            }
        };
    }

    inline void powerDownProcedure(std::shared_ptr<PowerOffContext> ctx)
    {
        startPowerDownTimer(ctx);
        initPowerDown(handleChassisStateChangeError(ctx),
                      handleChassisStateChangeSuccess(ctx));
    }

    inline void startPowerDownTimer(std::shared_ptr<PowerOffContext> ctx)
    {
        ctx->powerDownTimer.expires_after(config.powerDownTimeout);
        ctx->powerDownTimer.async_wait([ctx](boost::system::error_code ec) {
            if (ec)
            {
                return;
            }
            ctx->powerDownTimeoutOccured = true;
            RedfishLogger::logPowerShutdownFailed();
            Logger::log<LogLevel::error>("Power shutdown failed");
            if (ctx->completionCallback)
            {
                ctx->completionCallback(boost::system::errc::errc_t::timed_out);
            }
        });
    }

    std::function<void(void)>
        handleChassisStateChangeSuccess(std::shared_ptr<PowerOffContext> ctx)
    {
        return [self = shared_from_this(), ctx]() {
            if (!ctx->powerDownTimeoutOccured)
            {
                self->getPlatformPowerState(
                    [self, ctx](boost::system::error_code ec,
                                std::string powerState) {
                        if (ec)
                        {
                            Logger::log<LogLevel::error>(
                                "Err %lu: DBus 'GetProperty' CurrentPowerState "
                                "call failed",
                                ec.value());
                            return;
                        }
                        if (powerState ==
                            "xyz.openbmc_project.State.Chassis.PowerState.Off")
                        {
                            ctx->powerDownTimer.cancel();
                            if (ctx->completionCallback)
                            {
                                ctx->completionCallback(
                                    boost::system::errc::errc_t::success);
                            }
                            return;
                        }
                        else
                        {
                            self->handleChassisStateChangeSuccess(ctx);
                        }
                    });
            }
        };
    }

    std::function<void(boost::system::error_code)>
        handleChassisStateChangeError(std::shared_ptr<PowerOffContext> ctx)
    {
        return [self = shared_from_this(), ctx](boost::system::error_code ec) {
            if (ec == boost::system::errc::not_supported)
            {
                ctx->powerDownTimer.cancel();
                RedfishLogger::logPowerShutdownFailed();
                Logger::log<LogLevel::error>("Power shutdown failed");
                if (ctx->completionCallback)
                {
                    ctx->completionCallback(
                        boost::system::errc::errc_t::not_supported);
                }
            }
            else
            {
                if (!ctx->powerDownTimeoutOccured)
                {
                    self->initPowerDown(
                        self->handleChassisStateChangeError(ctx),
                        self->handleChassisStateChangeSuccess(ctx));
                }
            }
        };
    }

    template <typename OnError, typename OnSuccess>
    inline void initSoftShutdown(OnError&& onError, OnSuccess&& onSuccess)
    {
        sdbusplus::asio::setProperty(
            *bus.get(), config.hostStateServiceName, config.hostStateObjectPath,
            config.kHostStateServiceInterface, "RequestedHostTransition",
            std::string("xyz.openbmc_project.State.Host.Transition.Off"),
            [onError, onSuccess](boost::system::error_code ec) {
                if (ec)
                {
                    onError(ec);
                }
                else
                {
                    onSuccess();
                }
            });
    }

    template <typename OnError, typename OnSuccess>
    inline void initPowerDown(OnError&& onError, OnSuccess&& onSuccess)
    {
        sdbusplus::asio::setProperty(
            *bus.get(), config.chassisStateServiceName,
            config.chassisStateObjectPath, config.kChassisStateServiceInterface,
            "RequestedPowerTransition",
            std::string("xyz.openbmc_project.State.Chassis.Transition.Off"),
            [onError, onSuccess](boost::system::error_code ec) {
                if (ec)
                {
                    onError(ec);
                }
                else
                {
                    onSuccess();
                }
            });
    }

    template <typename OnSuccess>
    inline void getPlatformPowerState(OnSuccess&& onSuccess)
    {
        sdbusplus::asio::getProperty<std::string>(
            *bus.get(), config.chassisStateServiceName,
            config.chassisStateObjectPath, config.kChassisStateServiceInterface,
            "CurrentPowerState", onSuccess);
    }
};

} // namespace nodemanager
