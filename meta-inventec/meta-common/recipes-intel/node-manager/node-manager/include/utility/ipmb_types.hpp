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

namespace nodemanager
{

static constexpr uint8_t kIpmiNetFnOem = 0x2E;
static constexpr uint8_t kIpmiNetFnApp = 0x06;
static constexpr uint8_t kIpmiGetNmCapabilitiesCmd = 0xDE;
static constexpr uint8_t kIpmiSetNmCapabilitiesCmd = 0xDD;
static constexpr uint8_t kIpmiColdResetCmd = 0x02;
static constexpr uint8_t kSupportedAndEnabledValue = 3;
static constexpr uint8_t kSupportedAndDisabledValue = 1;

#pragma pack(push, 1)
struct Iana
{
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
};
static constexpr Iana IntelIana = {0x57, 0x01, 0x00};

struct ProxyCapabilities
{
    uint8_t peci : 2;
    uint8_t pmBus : 2;
    uint8_t ipmb : 2;
    uint8_t micDiscovery : 2;
};
static constexpr ProxyCapabilities noProxyCapabilities = {0, 0, 0, 0};

struct AssistModuleCapabilities
{
    uint8_t nm : 2;
    uint8_t bios : 2;
    uint8_t ras : 2;
    uint8_t performance : 2;
};
#pragma pack(pop)

//------------------------------------------------------------------------------
// request section--------------------------------------------------------------
//------------------------------------------------------------------------------

namespace request
{
#pragma pack(push, 1)
struct GetCapabilities
{
    const Iana iana = IntelIana;
    uint32_t reserved = 0;
    uint8_t ipmiVersion = 2;
    uint16_t ipmiSupportedSize = 0x88;
    ProxyCapabilities proxy = noProxyCapabilities;
};

struct SetCapabilities
{
    const Iana iana = IntelIana;
    uint8_t reserved1 = 0;
    uint64_t reserved8 = 0;
    AssistModuleCapabilities assistModule;
};
#pragma pack(pop)
} // namespace request

//------------------------------------------------------------------------------
// response section-------------------------------------------------------------
//------------------------------------------------------------------------------
namespace response
{
#pragma pack(push, 1)
struct GetCapabilities
{
    const Iana iana = IntelIana;
    uint8_t majorFwVersion : 7, reserved : 1;
    uint8_t minorFwVersion;
    uint8_t fwBuildAB;
    uint8_t fwBuildC;
    uint8_t ipmiVersion;
    uint16_t ipmiSupportedSize;
    uint8_t stateControl;
    ProxyCapabilities proxy;
    AssistModuleCapabilities assistModule;
};
struct SetCapabilities
{
    const Iana iana = IntelIana;
    uint8_t majorFwVersion : 7, reserved : 1;
    uint8_t minorFwVersion;
    uint8_t fwBuildAB;
    uint8_t fwBuildC;
    uint8_t ipmiVersion;
    uint16_t ipmiSupportedSize;
    uint8_t stateControl;
    ProxyCapabilities proxy;
    AssistModuleCapabilities assistModule;
};
#pragma pack(pop)
} // namespace response

} // namespace nodemanager