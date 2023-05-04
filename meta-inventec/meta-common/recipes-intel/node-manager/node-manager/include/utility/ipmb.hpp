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

#include "ipmb_types.hpp"

#include <iostream>
#include <sdbusplus/asio/object_server.hpp>

namespace nodemanager
{

constexpr const sdbusplus::SdBusDuration kIpmbTimeout =
    sdbusplus::SdBusDuration{1000000};
constexpr const char* kIpmbBus = "xyz.openbmc_project.Ipmi.Channel.Ipmb";
constexpr const char* kObjectPath = "/xyz/openbmc_project/Ipmi/Channel/Ipmb";
constexpr const char* kIpmbInterf = "org.openbmc.Ipmb";
// IpmbDbusRspType tuple: status, netfn, lun, cmd, cc, array of payload
using IpmbDbusRspType =
    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>;

class IpmbUtil
{
  public:
    IpmbUtil() = delete;
    IpmbUtil(const IpmbUtil&) = delete;
    IpmbUtil& operator=(const IpmbUtil&) = delete;
    IpmbUtil(IpmbUtil&&) = delete;
    IpmbUtil& operator=(IpmbUtil&&) = delete;
    ~IpmbUtil() = default;

    /**
     * @brief This function sends ipmi ColdReset to ME.
     *
     * @param bus
     * @return true if ipmi message was send with success
     * @return false
     */
    static bool
        ipmbSendColdRequest(std::shared_ptr<sdbusplus::asio::connection> bus)
    {
        if (auto res = ipmbSendBytes(bus, std::vector<uint8_t>(), kIpmiNetFnApp,
                                     kIpmiColdResetCmd, kIpmbTimeout))
        {
            return true;
        }
        return false;
    }

    /**
     * @brief Generic function that can be used to send ipmi message that has
     * some Request payload and expects some Response data.
     *
     * @tparam Response - payload returned with response (dosn't not include the
     * complitionCode)
     * @tparam Request - payload to send
     * @param bus
     * @param req
     * @param netFn - net function
     * @param cmd - ipmi command
     * @return std::optional<Response> if command succeed and completion code is
     * OK then returned ipmi payload is mapped to the Response structure and
     * returned.
     */
    template <class Response, class Request>
    static std::optional<Response> ipmbSendRequest(
        std::shared_ptr<sdbusplus::asio::connection> bus, const Request& req,
        uint8_t netFn, uint8_t cmd,
        std::optional<sdbusplus::SdBusDuration> timeout = kIpmbTimeout)
    {
        // convert req to vector of bytes
        auto const ptr = reinterpret_cast<const uint8_t*>(&req);
        std::vector<uint8_t> dataToSend = std::vector(ptr, ptr + sizeof(req));

        if (auto ipmbResponse =
                ipmbSendBytes(bus, dataToSend, netFn, cmd, timeout))
        {
            Response res{};
            const auto dataReceived =
                std::get<std::vector<uint8_t>>(*ipmbResponse);
            if (dataReceived.size() != sizeof(res))
            {
                Logger::log<LogLevel::error>(
                    "Invalid response size, provided: %d while expected: %d",
                    dataReceived.size(), sizeof(res));
                return std::nullopt;
            }
            std::copy(dataReceived.begin(), dataReceived.end(),
                      reinterpret_cast<char*>(&res));
            return res;
        }
        return std::nullopt;
    }

  private:
    static std::optional<IpmbDbusRspType>
        ipmbSendBytes(std::shared_ptr<sdbusplus::asio::connection> bus,
                      std::vector<uint8_t> dataToSend, uint8_t netFn,
                      uint8_t cmd,
                      std::optional<sdbusplus::SdBusDuration> timeout)
    {
        auto mesg = bus->new_method_call(kIpmbBus, kObjectPath, kIpmbInterf,
                                         "sendRequest");
        mesg.append(kIpmbChannelNumber, netFn, kIpmiLun, cmd, dataToSend);
        IpmbDbusRspType ipmbResponse;
        try
        {
            auto ret = bus->call(mesg, timeout);
            if (ret.is_method_error())
            {
                Logger::log<LogLevel::error>(
                    "sendRequest failed with errno: 0x%X", ret.get_errno());
                return std::nullopt;
            }
            ret.read(ipmbResponse);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            Logger::log<LogLevel::error>(
                "Cannot send message via ipmid, error: %s", e.what());
            return std::nullopt;
        }

        const auto compCode = std::get<4>(ipmbResponse);
        if (compCode != 0x00)
        {
            Logger::log<LogLevel::error>(
                "sendRequest get response with complitionCode: 0x%X",
                static_cast<unsigned>(compCode));
            return std::nullopt;
        }
        return ipmbResponse;
    }

    static constexpr uint8_t kIpmbChannelNumber = 1U;
    static constexpr uint8_t kIpmiLun = 0;
};
} // namespace nodemanager