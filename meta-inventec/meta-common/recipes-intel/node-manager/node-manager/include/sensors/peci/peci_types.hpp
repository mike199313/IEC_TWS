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

#include <sstream>

#include "peci.h"

namespace nodemanager
{

constexpr unsigned COMPLETION_CODE_SUCCESS = 0x40;
constexpr auto PECI_TRANSPORT_CPU0_ADDRESS = 0x30;
static constexpr uint8_t turboRatioCoreCount = 4U;

//------------------------------------------------------------------------------
// request section--------------------------------------------------------------
//------------------------------------------------------------------------------

namespace request
{
#pragma pack(push, 1)

struct CpuType
{
    uint32_t stepping : 4, model : 4, family : 4, type : 1, reserved1 : 3,
        extModel : 4, extFamily : 8, reserved2 : 4;
};

struct CpuId
{
    uint8_t model;
    uint8_t family;
    uint8_t extModel;
    uint8_t extFamily;
    uint8_t steppingHcc;
};

static constexpr CpuId CpuIdIcx = {0x0A, 0x06, 0x06, 0x00, 0x04};
static constexpr CpuId CpuIdSpr = {0x0F, 0x06, 0x08, 0x00, 0x00};
static constexpr CpuId CpuIdGnr = {0x0D, 0x06, 0x0A, 0x00, 0x00};

enum class CpuModelType
{
    unknown,
    icxLCC,
    icxHCC,
    spr,
    gnr
};

static inline CpuModelType getCpuModel(uint32_t rawCpuId)
{
    auto cpuId = reinterpret_cast<CpuType*>(&rawCpuId);

    if ((cpuId->model == CpuIdIcx.model) &&
        (cpuId->family == CpuIdIcx.family) &&
        (cpuId->extModel == CpuIdIcx.extModel) &&
        (cpuId->extFamily == CpuIdIcx.extFamily))
    {
        if (cpuId->stepping >= CpuIdIcx.steppingHcc)
        {
            return CpuModelType::icxHCC;
        }
        else
        {
            return CpuModelType::icxLCC;
        }
    }
    else if ((cpuId->model == CpuIdSpr.model) &&
             (cpuId->family == CpuIdSpr.family) &&
             (cpuId->extModel == CpuIdSpr.extModel) &&
             (cpuId->extFamily == CpuIdSpr.extFamily))
    {
        return CpuModelType::spr;
    }
    else if ((cpuId->model == CpuIdGnr.model) &&
             (cpuId->family == CpuIdGnr.family) &&
             (cpuId->extModel == CpuIdGnr.extModel) &&
             (cpuId->extFamily == CpuIdGnr.extFamily))
    {
        return CpuModelType::gnr;
    }
    else
    {
        throw std::runtime_error("Unknown CPUID");
    }

    return CpuModelType::unknown;
}

struct EndpointHeader
{
    uint8_t messageType;
    uint8_t eid;
    uint8_t fid;
    uint8_t bar;
    uint8_t addressType;
    uint8_t seg;
};
struct EndpointConfigPci
{
    EndpointHeader header;
    uint32_t reg : 12, func : 3, dev : 5, bus : 8, reserved : 4;
};
struct PeciHeader
{
    uint8_t command;
    uint8_t hostId;
};
static constexpr PeciHeader RdEndpointConfigHeader = {0xC1, 0};
static constexpr PeciHeader WrEndpointConfigHeader = {0xC5, 0};
static constexpr EndpointHeader EndpointConfigExtHeaderPciLocal = {3, 0, 0,
                                                                   0, 4, 0};
struct PkgConfig
{
    uint8_t index;
    uint16_t param;
};
static constexpr PeciHeader RdPkgConfigHeader = {0xA1, 0};
static constexpr PeciHeader WrPkgConfigHeader = {0xA5, 0};
struct GetCpuC0Counter
{
    const PeciHeader header = RdPkgConfigHeader;
    PkgConfig payload = {0x1F, 0xFE};
};

struct GetCpuEpiCounter
{
    const PeciHeader header = RdPkgConfigHeader;
    PkgConfig payload = {0x06, 0x0000};
};

struct GetCpuId
{
    const PeciHeader header = RdPkgConfigHeader;
    PkgConfig payload = {0, 0};
};

struct GetCpuDieMask
{
    const PeciHeader header = RdPkgConfigHeader;
    PkgConfig payload = {0, 7};
};

struct GetCapabilityRegister
{
    GetCapabilityRegister(uint32_t cpuId)
    {
        switch (getCpuModel(cpuId))
        {
            case CpuModelType::icxLCC:
            case CpuModelType::icxHCC:
            case CpuModelType::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x94, 3, 30, 31, 0};
                break;
            case CpuModelType::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x294, 0, 5, 30, 0};
                break;
            default:
                throw std::runtime_error(
                    "The CPU model returned by the getCpuModel() not covered "
                    "in the GetCapabilityRegister()");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload;
};

struct GetCoreMaskLow
{
    GetCoreMaskLow(uint32_t cpuId)
    {
        switch (getCpuModel(cpuId))
        {
            case CpuModelType::icxLCC:
            case CpuModelType::icxHCC:
                payload = {EndpointConfigExtHeaderPciLocal, 0xD0, 3, 30, 31, 0};
                break;
            case CpuModelType::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x80, 6, 30, 31, 0};
                break;
            case CpuModelType::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x488, 0, 5, 30, 0};
                break;
            default:
                throw std::runtime_error(
                    "The CPU model returned by the getCpuModel() not covered "
                    "in the GetCoreMaskLow()");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload;
};
struct GetCoreMaskHigh
{
    GetCoreMaskHigh(uint32_t cpuId)
    {
        switch (getCpuModel(cpuId))
        {
            case CpuModelType::icxLCC:
            case CpuModelType::icxHCC:
                payload = {EndpointConfigExtHeaderPciLocal, 0xD4, 3, 30, 31, 0};
                break;
            case CpuModelType::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x84, 6, 30, 31, 0};
                break;
            case CpuModelType::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x48C, 0, 5, 30, 0};
                break;
            default:
                throw std::runtime_error(
                    "The CPU model returned by the getCpuModel() not covered "
                    "in the GetCoreMaskHigh()");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload;
};

struct TurboRatioPkgConfigDefault
{
    uint8_t index;
    uint16_t limitSource : 1, avxLevel : 4, coreCount : 7, reserved : 4;
};
struct TurboRatioPkgConfigGnr
{
    uint8_t index;
    uint16_t hiLowSelect : 1, turboRatioLimit : 1, limitSource : 1,
        avxLevel : 3, reserved : 10;
};
union TurboRatioPkgConfigUnion
{
    TurboRatioPkgConfigDefault defaultCpu;
    TurboRatioPkgConfigGnr gnr;
};
struct GetTurboRatio
{
    GetTurboRatio(uint32_t cpuId)
    {
        switch (getCpuModel(cpuId))
        {
            case CpuModelType::icxLCC:
            case CpuModelType::icxHCC:
            case CpuModelType::spr:
                payload.defaultCpu = {49, 0, 0, 0, 0};
                break;
            case CpuModelType::gnr:
                payload.gnr = {49, 0, 0, 0, 0, 0};
                break;
            default:
                throw std::runtime_error(
                    "The CPU model returned by the getCpuModel() not covered "
                    "in the GetTurboRatio()");
        }
    }
    const PeciHeader header = RdPkgConfigHeader;
    TurboRatioPkgConfigUnion payload;
};

struct GetPlatformInfoLow
{
    GetPlatformInfoLow(uint32_t cpuId)
    {
        switch (getCpuModel(cpuId))
        {
            case CpuModelType::icxLCC:
            case CpuModelType::icxHCC:
            case CpuModelType::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0xA8, 0, 30, 31, 0};
                break;
            case CpuModelType::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x138, 0, 5, 30, 0};
                break;
            default:
                throw std::runtime_error(
                    "The CPU model returned by the getCpuModel() not covered"
                    "in the GetPlatformInfoLow()");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload;
};

struct GetTurboRatioLimitPkgConfig
{
    uint8_t index;
    uint16_t reserved0;
};
struct GetTurboRatioLimit
{
    const PeciHeader header = RdPkgConfigHeader;
    GetTurboRatioLimitPkgConfig payload = {50, 0};
};

struct SetTurboRatioLimitPkgConfig
{
    uint8_t index;
    uint16_t reserved0;
    uint8_t ratioLimit;
    uint16_t reserved1;
    uint8_t reserved2;
    uint8_t checksum;
};

struct SetTurboRatioLimit
{
    const PeciHeader header = WrPkgConfigHeader;
    SetTurboRatioLimitPkgConfig payload = {50, 0, 0, 0, 0, 0};
};

struct GetPlatformInfoHigh
{
    GetPlatformInfoHigh(uint32_t cpuId)
    {
        switch (getCpuModel(cpuId))
        {
            case CpuModelType::icxLCC:
            case CpuModelType::icxHCC:
            case CpuModelType::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0xAC, 0, 30, 31, 0};
                break;
            case CpuModelType::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x13C, 0, 5, 30, 0};
                break;
            default:
                throw std::runtime_error(
                    "The CPU model returned by the getCpuModel() not covered"
                    "in the GetPlatformInfoHigh()");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload;
};

struct SetHwpmPreferencePkgConfig
{
    const PeciHeader header = WrPkgConfigHeader;
    const PkgConfig pkgConfig = {0x35, 0x01};
    uint32_t data;
    uint8_t checksum;
};

struct SetHwpmPreferenceBiasPkgConfig
{
    const PeciHeader header = WrPkgConfigHeader;
    const PkgConfig pkgConfig = {0x35, 0x02};
    uint32_t data;
    uint8_t checksum;
};

struct SetHwpmPreferenceOverridePkgConfig
{
    const PeciHeader header = WrPkgConfigHeader;
    const PkgConfig pkgConfig = {0x35, 0x03};
    uint32_t data;
    uint8_t checksum;
};

struct GetProchotRatio
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0xB0, 0x02, 30, 31, 0};
};

struct SetProchotRatioPayload
{
    EndpointConfigPci endpointConfig = {
        EndpointConfigExtHeaderPciLocal, 0xB0, 0x02, 30, 31, 0};
    uint32_t prochot_ratio : 8, reserved0 : 24;
    uint8_t checksum;
};

struct SetProchotRatio
{
    const PeciHeader header = WrEndpointConfigHeader;
    SetProchotRatioPayload payload;
};

#pragma pack(pop)
} // namespace request

//------------------------------------------------------------------------------
// response section-------------------------------------------------------------
//------------------------------------------------------------------------------
namespace response
{
#pragma pack(push, 1)

struct GetCpuC0Counter
{
    uint8_t compCode;
    uint64_t c0Counter;
};

struct GetCpuEpiCounter
{
    uint8_t compCode;
    uint64_t epiCounter;
};

struct GetCpuId
{
    uint8_t compCode;
    uint32_t cpuId;
};

struct GetCpuDieMask
{
    uint8_t compCode;
    uint32_t mask;
};

struct GetCapabilityRegister
{
    uint8_t compCode;
    // uint32_t unused : 26, energyEfficientTurbo : 1;
    uint32_t capabilities;
};
struct GetCoreMask
{
    uint8_t compCode;
    uint32_t coreMask;
};
struct GetTurboRatio
{
    uint8_t compCode;
    uint8_t ratio[turboRatioCoreCount];
};
struct GetPlatformInfoLow
{
    uint8_t compCode;
    uint8_t reserved0;
    uint8_t max_non_turbo_ratio;
    uint8_t reserved1;
    uint8_t reserved2;
};
struct GetTurboRatioLimit
{
    uint8_t compCode;
    uint8_t ratioLimit;
    uint16_t reserved0;
    uint8_t reserved1;
};

struct SetTurboRatioLimit
{
    uint8_t compCode;
};

struct GetPlatformInfoHigh
{
    uint8_t compCode;
    uint8_t reserved0;
    uint8_t max_efficiency_ratio;
    uint8_t min_operating_ratio;
    uint8_t reserved1;
};

struct SetHwpmPreference
{
    uint8_t compCode;
};

struct SetHwpmPreferenceBias
{
    uint8_t compCode;
};

struct SetHwpmPreferenceOverride
{
    uint8_t compCode;
};

struct GetProchotRatio
{
    uint8_t compCode;
    uint32_t prochot_ratio : 8, reserved0 : 24;
};

struct SetProchotRatio
{
    uint8_t compCode;
};

#pragma pack(pop)
} // namespace response

} // namespace nodemanager
