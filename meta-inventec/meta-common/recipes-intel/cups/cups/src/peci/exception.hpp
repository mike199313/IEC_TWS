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

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace cups
{

namespace peci
{

class Exception : public std::runtime_error
{
  public:
    Exception() : std::runtime_error("PECI exception")
    {
        // TODO: Find all occurrences and replace with meaningful message
    }

    Exception(const std::string& what) : std::runtime_error(what)
    {}

    Exception(const uint8_t address, const std::string& what) :
        std::runtime_error(toHex(address) + " : " + what)
    {}

  private:
    static std::string toHex(const uint8_t value)
    {
        std::stringstream ss;
        ss << "0x" << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(value);
        return ss.str();
    }
};

} // namespace peci

#define PECI_EXCEPTION(what)                                                   \
    (peci::Exception(std::string(__func__) + "():" +                           \
                     std::to_string(__LINE__) + " > " + std::string(what)))

#define PECI_EXCEPTION_ADDR(addr, what)                                        \
    (peci::Exception((addr), std::string(__func__) +                           \
                                 "():" + std::to_string(__LINE__) + " > " +    \
                                 std::string(what)))
} // namespace cups
