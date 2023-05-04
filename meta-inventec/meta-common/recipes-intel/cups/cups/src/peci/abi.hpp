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

#include "peci/exception.hpp"
#include "utils/log.hpp"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>

namespace cups
{

namespace peci
{

namespace cpu
{

static constexpr uint32_t limit = 8;
static constexpr uint32_t minAddress = 0x30;

static inline bool isPrimary(const uint32_t address)
{
    return address == minAddress;
}

namespace id
{

struct type
{
    uint32_t stepping : 4, model : 4, family : 4, type : 1, reserved1 : 3,
        extModel : 4, extFamily : 8, reserved2 : 4;
};

struct spr
{
    static constexpr uint8_t model = 0x0F;
    static constexpr uint8_t family = 0x06;
    static constexpr uint8_t extModel = 0x08;
    static constexpr uint8_t extFamily = 0x00;
};

struct gnr
{
    static constexpr uint8_t model = 0x0D;
    static constexpr uint8_t family = 0x06;
    static constexpr uint8_t extModel = 0x0A;
    static constexpr uint8_t extFamily = 0x00;
};

} // namespace id

enum class model
{
    unknown,
    spr,
    gnr
};

template <class T>
static inline bool checkModel(const id::type* cpuId)
{
    return (cpuId->model == T::model) && (cpuId->family == T::family) &&
           (cpuId->extModel == T::extModel) &&
           (cpuId->extFamily == T::extFamily);
}

static inline model toModel(uint32_t rawCpuId)
{
    auto cpuId = reinterpret_cast<id::type*>(&rawCpuId);

    if (checkModel<id::spr>(cpuId))
    {
        return model::spr;
    }
    else if (checkModel<id::gnr>(cpuId))
    {
        return model::gnr;
    }
    else
    {
        throw PECI_EXCEPTION("Unknown CPUID == " + utils::toHex(rawCpuId));
    }

    return model::unknown;
}

} // namespace cpu

#pragma pack(1)
namespace abi
{

namespace iio
{

static constexpr unsigned dwordsInGigabyte = 250000000;
static constexpr unsigned fullDuplex = 2;

// PerfMon configuration
// TODO: Use dedicated types in adapter calls
static constexpr uint32_t xppMr = 0x100800;
static constexpr uint32_t xppMer = 0x1E011F;
static constexpr uint32_t xppErConf = 0x2;

struct Port
{
    enum class Type
    {
        DMI,
        PCI
    };

    uint8_t bus; // Bus number in B/D/F addressing
    Type type;

    // TODO: Rework data structures so this won't be necessary
    bool isDualUse = false; // can be either DMI or PCI

    std::string toString() const
    {
        std::stringstream ss;

        ss << "Bus: " << static_cast<int>(bus);

        ss << " (";
        switch (type)
        {
            case Type::DMI:
                ss << "DMI";
                break;
            case Type::PCI:
                ss << "PCI";
                break;
            default:
                ss << "raw: " << static_cast<int>(type);
        }
        ss << ")";

        return ss.str();
    }
};

struct Controller
{
    enum class Gen
    {
        Gen3,
        Gen4,
        Gen5
    };

    uint8_t device; // Device number in B/D/F addressing
    Gen gen;

    // TODO: Rework data structures so this won't be necessary
    bool canBeDmi = false;

    std::string toString() const
    {
        std::stringstream ss;

        ss << "Controller: " << static_cast<int>(device);

        ss << " (";
        switch (gen)
        {
            case Gen::Gen3:
                ss << "Gen3";
                break;
            case Gen::Gen4:
                ss << "Gen4";
                break;
            case Gen::Gen5:
                ss << "Gen5";
                break;
            default:
                ss << "raw: " << static_cast<int>(gen);
        }
        ss << ")";

        return ss.str();
    }
};

namespace spr
{

namespace reg
{
static constexpr uint16_t linkStatus = 0x52;
} // namespace reg

namespace gen4
{
static constexpr std::array<Controller, 4> links = {
    {{8, Controller::Gen::Gen4},
     {6, Controller::Gen::Gen4},
     {4, Controller::Gen::Gen4},
     {2, Controller::Gen::Gen4}}};
} // namespace gen4

namespace gen5
{
static constexpr std::array<Controller, 4> links = {
    {{1, Controller::Gen::Gen5},
     {3, Controller::Gen::Gen5},
     {5, Controller::Gen::Gen5},
     {7, Controller::Gen::Gen5}}};
} // namespace gen5

// TODO: Fix naming (Port and Link are probably IP and Port)
static constexpr std::array<Port, 6> ports = {{{0, Port::Type::PCI},
                                               {1, Port::Type::PCI},
                                               {2, Port::Type::PCI},
                                               {3, Port::Type::PCI},
                                               {4, Port::Type::PCI},
                                               {5, Port::Type::PCI}}};

static constexpr Port dualUsePort = {0, Port::Type::PCI, true};
static constexpr Controller possibleDmi = {8, Controller::Gen::Gen4, true};

static constexpr std::array<std::tuple<Port, Controller, uint16_t>, 48>
    linksMap = {{// Port 0
                 {dualUsePort, possibleDmi, reg::linkStatus},
                 {dualUsePort, gen4::links[1], reg::linkStatus},
                 {dualUsePort, gen4::links[2], reg::linkStatus},
                 {dualUsePort, gen4::links[3], reg::linkStatus},

                 {ports[0], gen5::links[0], reg::linkStatus},
                 {ports[0], gen5::links[1], reg::linkStatus},
                 {ports[0], gen5::links[2], reg::linkStatus},
                 {ports[0], gen5::links[3], reg::linkStatus},

                 // Port 1
                 {ports[1], gen4::links[0], reg::linkStatus},
                 {ports[1], gen4::links[1], reg::linkStatus},
                 {ports[1], gen4::links[2], reg::linkStatus},
                 {ports[1], gen4::links[3], reg::linkStatus},

                 {ports[1], gen5::links[0], reg::linkStatus},
                 {ports[1], gen5::links[1], reg::linkStatus},
                 {ports[1], gen5::links[2], reg::linkStatus},
                 {ports[1], gen5::links[3], reg::linkStatus},

                 // Port 2
                 {ports[2], gen4::links[0], reg::linkStatus},
                 {ports[2], gen4::links[1], reg::linkStatus},
                 {ports[2], gen4::links[2], reg::linkStatus},
                 {ports[2], gen4::links[3], reg::linkStatus},

                 {ports[2], gen5::links[0], reg::linkStatus},
                 {ports[2], gen5::links[1], reg::linkStatus},
                 {ports[2], gen5::links[2], reg::linkStatus},
                 {ports[2], gen5::links[3], reg::linkStatus},

                 // Port 3
                 {ports[3], gen4::links[0], reg::linkStatus},
                 {ports[3], gen4::links[1], reg::linkStatus},
                 {ports[3], gen4::links[2], reg::linkStatus},
                 {ports[3], gen4::links[3], reg::linkStatus},

                 {ports[3], gen5::links[0], reg::linkStatus},
                 {ports[3], gen5::links[1], reg::linkStatus},
                 {ports[3], gen5::links[2], reg::linkStatus},
                 {ports[3], gen5::links[3], reg::linkStatus},

                 // Port 4
                 {ports[4], gen4::links[0], reg::linkStatus},
                 {ports[4], gen4::links[1], reg::linkStatus},
                 {ports[4], gen4::links[2], reg::linkStatus},
                 {ports[4], gen4::links[3], reg::linkStatus},

                 {ports[4], gen5::links[0], reg::linkStatus},
                 {ports[4], gen5::links[1], reg::linkStatus},
                 {ports[4], gen5::links[2], reg::linkStatus},
                 {ports[4], gen5::links[3], reg::linkStatus},

                 // Port 5
                 {ports[5], gen4::links[0], reg::linkStatus},
                 {ports[5], gen4::links[1], reg::linkStatus},
                 {ports[5], gen4::links[2], reg::linkStatus},
                 {ports[5], gen4::links[3], reg::linkStatus},

                 {ports[5], gen5::links[0], reg::linkStatus},
                 {ports[5], gen5::links[1], reg::linkStatus},
                 {ports[5], gen5::links[2], reg::linkStatus},
                 {ports[5], gen5::links[3], reg::linkStatus}}};

} // namespace spr

namespace gnr
{

namespace reg
{
static constexpr uint16_t linkStatus = 0x52;
} // namespace reg

namespace gen4
{
static constexpr std::array<Controller, 4> links = {
    {{8, Controller::Gen::Gen4},
     {6, Controller::Gen::Gen4},
     {4, Controller::Gen::Gen4},
     {2, Controller::Gen::Gen4}}};
} // namespace gen4

namespace gen5
{
static constexpr std::array<Controller, 4> links = {
    {{1, Controller::Gen::Gen5},
     {3, Controller::Gen::Gen5},
     {5, Controller::Gen::Gen5},
     {7, Controller::Gen::Gen5}}};
} // namespace gen5

// TODO: Fix naming (Port and Link are probably IP and Port)
static constexpr std::array<Port, 6> ports = {{{0, Port::Type::PCI},
                                               {1, Port::Type::PCI},
                                               {2, Port::Type::PCI},
                                               {3, Port::Type::PCI},
                                               {4, Port::Type::PCI},
                                               {5, Port::Type::PCI}}};

static constexpr Port dualUsePort = {0, Port::Type::PCI, true};
static constexpr Controller possibleDmi = {8, Controller::Gen::Gen4, true};

static constexpr std::array<std::tuple<Port, Controller, uint16_t>, 48>
    linksMap = {{// Port 0
                 {dualUsePort, possibleDmi, reg::linkStatus},
                 {dualUsePort, gen4::links[1], reg::linkStatus},
                 {dualUsePort, gen4::links[2], reg::linkStatus},
                 {dualUsePort, gen4::links[3], reg::linkStatus},

                 {ports[0], gen5::links[0], reg::linkStatus},
                 {ports[0], gen5::links[1], reg::linkStatus},
                 {ports[0], gen5::links[2], reg::linkStatus},
                 {ports[0], gen5::links[3], reg::linkStatus},

                 // Port 1
                 {ports[1], gen4::links[0], reg::linkStatus},
                 {ports[1], gen4::links[1], reg::linkStatus},
                 {ports[1], gen4::links[2], reg::linkStatus},
                 {ports[1], gen4::links[3], reg::linkStatus},

                 {ports[1], gen5::links[0], reg::linkStatus},
                 {ports[1], gen5::links[1], reg::linkStatus},
                 {ports[1], gen5::links[2], reg::linkStatus},
                 {ports[1], gen5::links[3], reg::linkStatus},

                 // Port 2
                 {ports[2], gen4::links[0], reg::linkStatus},
                 {ports[2], gen4::links[1], reg::linkStatus},
                 {ports[2], gen4::links[2], reg::linkStatus},
                 {ports[2], gen4::links[3], reg::linkStatus},

                 {ports[2], gen5::links[0], reg::linkStatus},
                 {ports[2], gen5::links[1], reg::linkStatus},
                 {ports[2], gen5::links[2], reg::linkStatus},
                 {ports[2], gen5::links[3], reg::linkStatus},

                 // Port 3
                 {ports[3], gen4::links[0], reg::linkStatus},
                 {ports[3], gen4::links[1], reg::linkStatus},
                 {ports[3], gen4::links[2], reg::linkStatus},
                 {ports[3], gen4::links[3], reg::linkStatus},

                 {ports[3], gen5::links[0], reg::linkStatus},
                 {ports[3], gen5::links[1], reg::linkStatus},
                 {ports[3], gen5::links[2], reg::linkStatus},
                 {ports[3], gen5::links[3], reg::linkStatus},

                 // Port 4
                 {ports[4], gen4::links[0], reg::linkStatus},
                 {ports[4], gen4::links[1], reg::linkStatus},
                 {ports[4], gen4::links[2], reg::linkStatus},
                 {ports[4], gen4::links[3], reg::linkStatus},

                 {ports[4], gen5::links[0], reg::linkStatus},
                 {ports[4], gen5::links[1], reg::linkStatus},
                 {ports[4], gen5::links[2], reg::linkStatus},
                 {ports[4], gen5::links[3], reg::linkStatus},

                 // Port 5
                 {ports[5], gen4::links[0], reg::linkStatus},
                 {ports[5], gen4::links[1], reg::linkStatus},
                 {ports[5], gen4::links[2], reg::linkStatus},
                 {ports[5], gen4::links[3], reg::linkStatus},

                 {ports[5], gen5::links[0], reg::linkStatus},
                 {ports[5], gen5::links[1], reg::linkStatus},
                 {ports[5], gen5::links[2], reg::linkStatus},
                 {ports[5], gen5::links[3], reg::linkStatus}}};

} // namespace gnr

} // namespace iio

namespace memory
{

static constexpr unsigned freqRatio133Mhz = 13333;
static constexpr unsigned freqRatio100Mhz = 10000;
static constexpr unsigned freqRatioDiv = 100;
static constexpr unsigned channelWidth = 8;
static constexpr unsigned cacheLineSize = 64;

enum class Controller
{
    mc0 = 26,
    mc1 = 27,
    mc2 = 28,
    mc3 = 29
};

namespace spr
{
enum DimmOffset
{
    ch0Dimm0 = 0x2080C,
    ch0Dimm1 = 0x20810,
    ch1Dimm0 = 0x2880C,
    ch1Dimm1 = 0x28810
};

static constexpr std::array<Controller, 4> controllers = {
    Controller::mc0, Controller::mc1, Controller::mc2, Controller::mc3};
static constexpr std::array<
    std::tuple<Controller, uint8_t, DimmOffset, uint8_t>,
    4 * controllers.max_size()>
    channelsMap = {{{controllers[0], 0, DimmOffset::ch0Dimm0, 0},
                    {controllers[0], 0, DimmOffset::ch0Dimm1, 0},
                    {controllers[0], 0, DimmOffset::ch1Dimm0, 1},
                    {controllers[0], 0, DimmOffset::ch1Dimm1, 1},

                    {controllers[1], 0, DimmOffset::ch0Dimm0, 2},
                    {controllers[1], 0, DimmOffset::ch0Dimm1, 2},
                    {controllers[1], 0, DimmOffset::ch1Dimm0, 3},
                    {controllers[1], 0, DimmOffset::ch1Dimm1, 3},

                    {controllers[2], 0, DimmOffset::ch0Dimm0, 4},
                    {controllers[2], 0, DimmOffset::ch0Dimm1, 4},
                    {controllers[2], 0, DimmOffset::ch1Dimm0, 5},
                    {controllers[2], 0, DimmOffset::ch1Dimm1, 5},

                    {controllers[3], 0, DimmOffset::ch0Dimm0, 6},
                    {controllers[3], 0, DimmOffset::ch0Dimm1, 6},
                    {controllers[3], 0, DimmOffset::ch1Dimm0, 7},
                    {controllers[3], 0, DimmOffset::ch1Dimm1, 7}}};

static constexpr std::array<std::tuple<uint16_t, uint16_t>, 4>
    controllersSampleIdxMap = {
        {{0x00, 0xE0}, {0x00, 0xE1}, {0x00, 0xE2}, {0x00, 0xE3}}};
} // namespace spr

namespace gnr
{
enum DimmOffset
{
    ch0Dimm0 = 0x40C,
    ch0Dimm1 = 0x410,
};

static constexpr std::array<std::tuple<uint8_t, uint8_t, DimmOffset, uint8_t>,
                            2 * 12>
    channelsMap = {
        {{5, 1, DimmOffset::ch0Dimm0, 0},  {5, 2, DimmOffset::ch0Dimm1, 0},
         {5, 3, DimmOffset::ch0Dimm0, 1},  {5, 4, DimmOffset::ch0Dimm1, 1},
         {5, 5, DimmOffset::ch0Dimm0, 2},  {5, 6, DimmOffset::ch0Dimm1, 2},
         {5, 7, DimmOffset::ch0Dimm0, 3},

         {6, 1, DimmOffset::ch0Dimm1, 3},  {6, 2, DimmOffset::ch0Dimm0, 4},
         {6, 3, DimmOffset::ch0Dimm1, 4},  {6, 4, DimmOffset::ch0Dimm0, 5},
         {6, 5, DimmOffset::ch0Dimm1, 5},  {6, 6, DimmOffset::ch0Dimm0, 6},
         {6, 7, DimmOffset::ch0Dimm1, 6},

         {7, 1, DimmOffset::ch0Dimm0, 7},  {7, 2, DimmOffset::ch0Dimm1, 7},
         {7, 3, DimmOffset::ch0Dimm0, 8},  {7, 4, DimmOffset::ch0Dimm1, 8},
         {7, 5, DimmOffset::ch0Dimm0, 9},  {7, 6, DimmOffset::ch0Dimm1, 9},
         {7, 7, DimmOffset::ch0Dimm0, 10},

         {8, 1, DimmOffset::ch0Dimm1, 10}, {8, 2, DimmOffset::ch0Dimm0, 11},
         {8, 3, DimmOffset::ch0Dimm1, 11}}};

static constexpr std::array<std::tuple<uint16_t, uint16_t>, 4>
    controllersSampleIdxMap = {
        {{0x00, 0xE0}, {0x00, 0xE1}, {0x00, 0xE2}, {0x00, 0xE3}}};
} // namespace gnr
} // namespace memory

static constexpr unsigned turboRatioCoreCount = 4U;

static constexpr unsigned cpuFreqRatioMHz = 100U;

static constexpr uint64_t MHzToHz(uint32_t mhz)
{
    return static_cast<uint64_t>(mhz) * 1000 * 1000;
}

// TODO: Use in calls
union XPPMRRegister
{
    uint32_t raw;
    struct
    {
        uint32_t cntrst : 1, ovs : 1, reserved0 : 1, pto : 2, pmssig : 1,
            cmpmd : 2, rstevsel : 3, cens : 3, cntmd : 2, evpolinv : 1,
            cntevsel : 2, egs : 2, ldes : 1, dfxlnsel : 2, reserved1 : 3,
            rstpulsen : 1, letcensel : 1, frcpmdaddz : 1;
    } bits;
};

// TODO: Use in calls
union XPPMERRegister
{
    uint32_t raw;
    struct
    {
        uint32_t txRxSel : 2, fccSel : 3, vcSel : 3, allVcSel : 1, hdSel : 1,
            qmSel : 2, reserved0 : 1, linkUtil : 4, xpResAssignment : 4,
            rxL0SateUtil : 1, txL0StateUtil : 1, countCe : 1, countUe : 1,
            l1StateUtil : 1;
    } bits;
};

// TODO: Use in calls
union XPPERConfRegister
{
    uint32_t raw;
    struct
    {
        uint32_t globalReset : 1, globalCounterEn : 1;
    } bits;
};

namespace request
{

struct PeciHeader
{
    uint8_t command;
    uint8_t hostId;
};

struct EndpointHeader
{
    uint8_t messageType;
    uint8_t eid;
    uint8_t fid;
    uint8_t bar;
    uint8_t addressType;
    uint8_t seg;
};

struct GetTelemetryHeader
{
    uint8_t version;
    uint8_t opcode;
};

struct PkgConfig
{
    uint8_t index;
    uint16_t param;
};

struct MaxTurboRatioPkgConfig
{
    uint8_t index;
    uint16_t limitSource : 1, avxLevel : 4, coreCount : 7, reserved : 4;
};

struct PciConfigLocal
{
    uint32_t reg : 12, func : 3, dev : 5, bus : 4;
};

struct EndpointConfigPci
{
    EndpointHeader header;
    uint32_t reg : 12, func : 3, dev : 5, bus : 8, reserved : 4;
};

struct EndpointConfigMmio32
{
    EndpointHeader header;
    uint8_t func : 3, dev : 5;
    uint8_t bus;
    uint32_t reg;
};

struct EndpointConfigMmio64
{
    EndpointHeader header;
    uint8_t func : 3, dev : 5;
    uint8_t bus;
    uint64_t reg;
};

struct GetTelemetrySample
{
    GetTelemetryHeader header;
    uint16_t aggregatorIndex;
    uint16_t sampleIndex;
};

static constexpr PeciHeader RdPkgConfigHeader = {0xA1, 0};
static constexpr PeciHeader RdEndpointConfigHeader = {0xC1, 0};
static constexpr PeciHeader WrEndpointConfigHeader = {0xC5, 0};
static constexpr PeciHeader GetTelemetryPeciHeader = {0x81, 0};

static constexpr EndpointHeader EndpointConfigExtHeaderPciLocal = {3, 0, 0,
                                                                   0, 4, 0};
static constexpr EndpointHeader EndpointConfigExtHeaderPci = {4, 0, 0, 0, 4, 0};
static constexpr EndpointHeader EndpointConfigExtHeaderMmio32 = {5, 0, 0,
                                                                 0, 5, 0};
static constexpr EndpointHeader EndpointConfigExtHeaderMmio64 = {5, 0, 0,
                                                                 0, 6, 0};

static constexpr GetTelemetryHeader GetTelemetrySampleHeader = {0, 0x02};

struct GetCpuId
{
    const PeciHeader header = RdPkgConfigHeader;
    PkgConfig payload = {0, 0};
};

struct GetCpuC0Counter
{
    const PeciHeader header = RdPkgConfigHeader;
    PkgConfig payload = {31, 0xFE};
};

struct GetMaxTurboRatio
{
    const PeciHeader header = RdPkgConfigHeader;
    MaxTurboRatioPkgConfig payload = {49, 0, 0, 0, 0};
};

struct GetCpuBusNumber
{
    GetCpuBusNumber(uint32_t cpuId)
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr: // the same address for gnr and spr
            case cpu::model::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0xD0, 2, 0, 30, 0};
                break;
            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload;
};

struct GetLinkStatus
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0, 0, 0, 0, 0};
};

struct GetXPPMonFrCtrClk
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x670, 0, 0, 0, 0};
};

struct GetXPPMonFrCtr
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x678, 0, 0, 0, 0};
};

struct GetXPPMR
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x594, 0, 2, 0, 0};
};

struct GetXPPMER
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x5AC, 0, 2, 0, 0};
};

struct GetXPPERConf
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x5C4, 0, 2, 0, 0};
};

struct GetXPPMdl
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x580, 0, 2, 0, 0};
};

struct GetXPPMdh
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x590, 0, 2, 0, 0};
};

struct GetDlu
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x4C0, 0, 3, 0, 0};
};

struct SetXPPMR
{
    const PeciHeader header = WrEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x594, 0, 2, 0, 0};
    XPPMRRegister reg;
    uint8_t checksum;
};

struct SetXPPMER
{
    const PeciHeader header = WrEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x5AC, 0, 2, 0, 0};
    XPPMERRegister reg;
    uint8_t checksum;
};

struct SetXPPERConf
{
    const PeciHeader header = WrEndpointConfigHeader;
    EndpointConfigPci payload = {
        EndpointConfigExtHeaderPciLocal, 0x5C4, 0, 2, 0, 0};
    XPPERConfRegister reg;
    uint8_t checksum;
};

struct GetCoreMaskLow
{
    GetCoreMaskLow(uint32_t cpuId)
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x80, 6, 30, 31, 0};
                break;
            case cpu::model::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x488, 0, 5, 30, 0};
                break;
            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload{};
};

struct GetCoreMaskHigh
{
    GetCoreMaskHigh(uint32_t cpuId)
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x84, 6, 30, 31, 0};
                break;
            case cpu::model::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x48C, 0, 5, 30, 0};
                break;
            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }

    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload{};
};

struct GetMaxNonTurboRatio
{
    GetMaxNonTurboRatio(uint32_t cpuId)
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0xA8, 0, 30, 31, 0};
                break;
            case cpu::model::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x138, 0, 5, 31, 0};
                break;
            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload{};
};

struct GetCapabilityRegister
{
    GetCapabilityRegister(uint32_t cpuId)
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x94, 3, 30, 31, 0};
                break;
            case cpu::model::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x294, 0, 5, 31, 0};
                break;
            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload{};
};

struct GetMemoryFreq
{
    GetMemoryFreq(uint32_t cpuId)
    {
        switch (cpu::toModel(cpuId))
        {
            case cpu::model::spr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x98, 1, 30, 31, 0};
                break;
            case cpu::model::gnr:
                payload = {EndpointConfigExtHeaderPciLocal, 0x1A0, 0, 5, 30, 0};
                break;
            default:
                throw PECI_EXCEPTION("Unhandled");
        }
    }
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigPci payload{};
};

struct GetDimmmtr32
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigMmio32 payload = {EndpointConfigExtHeaderMmio32, 0, 0, 0, 0};
};

struct GetMemoryRdCounter32
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigMmio32 payload = {EndpointConfigExtHeaderMmio32, 0, 0, 0,
                                    0x2290};
};

struct GetMemoryWrCounter32
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigMmio32 payload = {EndpointConfigExtHeaderMmio32, 0, 0, 0,
                                    0x2298};
};

struct GetDimmmtr64
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigMmio64 payload = {EndpointConfigExtHeaderMmio64, 0, 0, 0, 0};
};

struct GetMemoryRdCounter64
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigMmio64 payload = {EndpointConfigExtHeaderMmio64, 0, 0, 0,
                                    0x2290};
};

struct GetMemoryWrCounter64
{
    const PeciHeader header = RdEndpointConfigHeader;
    EndpointConfigMmio64 payload = {EndpointConfigExtHeaderMmio64, 0, 0, 0,
                                    0x2298};
};

struct GetMemoryRwCounter
{
    GetMemoryRwCounter(uint16_t aggregatorIdx, uint16_t sampleIdx)
    {
        payload = {GetTelemetrySampleHeader, aggregatorIdx, sampleIdx};
    }

    const PeciHeader header = GetTelemetryPeciHeader;
    GetTelemetrySample payload;
};

} // namespace request

namespace response
{

struct GetCpuId
{
    uint8_t compCode;
    uint32_t cpuId;
};

struct GetCpuC0Counter
{
    uint8_t compCode;
    uint64_t c0Counter;
};

struct GetCoreMask
{
    uint8_t compCode;
    uint32_t coreMask;
};

struct GetMaxNonTurboRatio
{
    uint8_t compCode;
    uint8_t reserved0;
    uint8_t ratio;
    uint8_t reserved1;
    uint8_t reserved2;
};

struct GetMaxTurboRatio
{
    uint8_t compCode;
    uint8_t ratio[turboRatioCoreCount];
};

struct GetCapabilityRegister
{
    uint8_t compCode;
    uint32_t unused : 26, energyEfficientTurbo : 1;
};

struct GetMemoryFreq
{
    uint8_t compCode;
    uint32_t frequency : 6, unused1 : 2, type : 4, unused2 : 20;
};

struct GetCpuBusNumber
{
    uint8_t compCode;
    uint8_t busNumber0;
    uint8_t busNumber1;
    uint8_t segment;
    uint8_t reserved;
};

struct GetDimmmtr
{
    uint8_t compCode;
    uint32_t caWidth : 2, raWidth : 3, ddr3Density : 3, ddr3Width : 2,
        baShrink : 1, unused0 : 1, rankCount : 2, unused1 : 1, dimmPop : 1,
        rankDisable : 4, unused2 : 1, ddr4Mode : 1, hdrl : 1, hdrlParity : 1,
        ddr4_3dNumRanksCs : 2;
};

struct GetMemoryCounter
{
    uint8_t compCode;
    uint64_t counter64bytes;
};

struct GetMemoryRwCounter
{
    uint8_t compCode;
    // Counter units are ::peci::abi::memory::cacheLineSize
    uint32_t cacheLineReads;
    uint32_t cacheLineWrites;
};

struct GetLinkStatus
{
    uint8_t compCode;
    uint32_t speed : 4, width : 6, unused0 : 3, active : 1, unused1 : 18;
};

struct SetXPPRegister
{
    uint8_t compCode;
};

struct GetXPPMR
{
    uint8_t compCode;
    XPPMRRegister reg;
};

struct GetXPPMER
{
    uint8_t compCode;
    XPPMERRegister reg;
};

struct GetXPPERConf
{
    uint8_t compCode;
    XPPERConfRegister reg;
};

struct GetXPPMdl
{
    uint8_t compCode;
    uint32_t counterDwords;
};

struct GetXPPMdh
{
    uint8_t compCode;
    uint32_t counterDwords0 : 4, reserved0 : 4, counterDwords1 : 4,
        reserved1 : 4;
};

struct GetDlu
{
    uint8_t compCode;
    uint8_t counterGigabytes[4];
};

struct GetXPPMonFrCtr
{
    uint8_t compCode;
    uint64_t count : 48, rsvd : 16;
};

} // namespace response

} // namespace abi
#pragma pack()

} // namespace peci

} // namespace cups
