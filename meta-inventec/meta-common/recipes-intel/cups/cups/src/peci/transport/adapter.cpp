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

#include "peci/transport/adapter.hpp"

#include "peci.h"

#include "peci/abi.hpp"
#include "utils/log.hpp"

#include <boost/container/flat_map.hpp>

#include <iomanip>
#include <sstream>

namespace cups
{

namespace peci
{

static_assert(cpu::limit == MAX_CPUS, "ABI verification failed");
static_assert(cpu::minAddress == MIN_CLIENT_ADDR, "ABI verification failed");

namespace transport
{

static std::string friendlyPeciStatus(const EPECIStatus status)
{
    static const boost::container::flat_map<EPECIStatus, std::string> names = {
        {PECI_CC_SUCCESS, "PECI_CC_SUCCESS"},
        {PECI_CC_INVALID_REQ, "PECI_CC_INVALID_REQ"},
        {PECI_CC_HW_ERR, "PECI_CC_HW_ERR"},
        {PECI_CC_DRIVER_ERR, "PECI_CC_DRIVER_ERR"},
        {PECI_CC_CPU_NOT_PRESENT, "PECI_CC_CPU_NOT_PRESENT"},
        {PECI_CC_MEM_ERR, "PECI_CC_MEM_ERR"},
        {PECI_CC_TIMEOUT, "PECI_CC_TIMEOUT"}};

    const auto it = names.find(status);
    if (it != names.end())
    {
        return it->second;
    }

    return "Unknown (" + std::to_string(status) + ")";
}

#if ENABLE_PECI_TRACE
static void trace(const std::string& name, const uint8_t* payload,
                  const size_t size)
{
    std::stringstream ss;
    ss << name << " : ";
    for (int i = 0; i < static_cast<int>(size); i++)
    {
        ss << "0x" << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(payload[i]) << " ";
    }
    LOG_DEBUG << ss.str();
}
#else
#define trace(name, payload, size)
#endif

bool commandHandler(const uint8_t target, const uint8_t* pReq,
                    const size_t reqSize, uint8_t* pRsp, const size_t rspSize,
                    const bool logError, const std::string& name)
{
    static constexpr unsigned devCompCodeSuccess = 0x40;

    EPECIStatus ret;

    trace(name, pReq, reqSize);

    ret = peci_raw(target, static_cast<uint8_t>(rspSize), pReq,
                   static_cast<uint8_t>(reqSize), pRsp,
                   static_cast<uint8_t>(rspSize));

    if (PECI_CC_SUCCESS != ret)
    {
        std::stringstream ss;
        ss << "Failed to execute PECI request " << name
           << ", peci_raw ret = " << friendlyPeciStatus(ret);
        if (logError)
        {
            LOG_ERROR << ss.str();
        }
        else
        {
            LOG_DEBUG << ss.str();
        }
        return false;
    }

    trace("Response", pRsp, rspSize);

    if (devCompCodeSuccess != pRsp[0])
    {
        std::stringstream ss;
        ss << "Invalid completion code for request " << name << ": 0x"
           << std::hex << static_cast<int>(pRsp[0]);
        if (logError)
        {
            LOG_ERROR << ss.str();
        }
        else
        {
            LOG_DEBUG << ss.str();
        }
        return false;
    }

    return true;
}

} // namespace transport

} // namespace peci

} // namespace cups
