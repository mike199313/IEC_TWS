#!/usr/bin/env python3
#
#  INTEL CONFIDENTIAL
#
#  Copyright 2020 Intel Corporation
#
#  This software and the related documents are Intel copyrighted materials,
#  and your use of them is governed by the express license under which they
#  were provided to you (License). Unless the License provides otherwise,
#  you may not use, modify, copy, publish, distribute, disclose or
#  transmit this software or the related documents without
#  Intel's prior written permission.
#
#  This software and the related documents are provided as is,
#  with no express or implied warranties, other than those
#  that are expressly stated in the License.

import argparse
import ctypes
import paramiko
import time

CPU_ID_ICX_LCC = 0x606A0
CPU_ID_ICX_HCC = 0x606A4
CPU_ID_SPR = 0x806f1  # TODO: Support for more variants

EXPECTED_XPP_MR = 0x100800
EXPECTED_XPP_MER = 0x1e011f
EXPECTED_XPPER_CONF = 0x2

CORES_IN_GET_TURBO_RESPONSE = 4
DWORDS_IN_GIGABYTE = 250000000
FULL_DUPLEX = 2
IIO_OVERFLOW = 0xFFFFFFFFF

PECI_HEADER_LEN = 3
PECI_MEMORY_CONTROLLER_BAR = 0
PECI_TRANSPORT_CPU0_ADDRESS = 0x30
PECI_TRANSPORT_DEFAULT_HOST_ID = 0

# RdPkgConfig
PECI_COMMAND_RD_PKG_CFG = 0xA1
PECI_TRANSPORT_RD_PKG_CFG_WR_LEN = 5
# WrPciCfgLocal
PECI_COMMAND_WR_PCI_CFG_LOC = 0xE5
PECI_TRANSPORT_WR_PCI_CFG_LOC_4B_WR_LEN = 0x0A
# RdEndpointCfgSpace
PECI_COMMAND_RD_ENDPOINT_CFG_SPACE = 0xC1
PECI_TRANSPORT_RD_ENDPOINT_CFG_LOC_PCI_CFG_WR_LEN = 12
PECI_TRANSPORT_RD_ENDPOINT_CFG_MMIO32_BDFBAR_WR_LEN = 14
PECI_TRANSPORT_RD_ENDPOINT_CFG_MMIO64_BDFBAR_WR_LEN = 18
PECI_TRANSPORT_ENDPOINT_CFG_LOC_PCI_CFG_MSG_TYPE = 3
PECI_TRANSPORT_ENDPOINT_CFG_LOC_PCI_CFG_ADDR_TYPE = 4

PECI_TRANSPORT_ENDPOINT_CFG_MMIO_BDFBAR_MSG_TYPE = 5
PECI_TRANSPORT_ENDPOINT_CFG_MMIO32_BDFBAR_ADDR_TYPE = 5
PECI_TRANSPORT_ENDPOINT_CFG_MMIO64_BDFBAR_ADDR_TYPE = 6

# WrEndpointCfg
PECI_COMMAND_WR_ENDPOINT_CFG_LOC_PCI = 0xC5
PECI_TRANSPORT_WR_ENDPOINT_CFG_LOC_PCI_4B_LEN = 0x11

# CPU Specific
# ICX
PECI_TRANSPORT_RD_END_CFG_CORE_MASK_FUNCTION_ICX = 0x3
PECI_TRANSPORT_RD_END_CFG_CORE_MASK_OFFSET_ICX = 0xD0
PECI_TRANSPORT_RD_PCI_CFG_BUS_NUMBER_OFFSET_ICX_LCC = 0XCC
PECI_TRANSPORT_RD_PCI_CFG_BUS_NUMBER_OFFSET_ICX_HCC = 0XD0

# SPR
PECI_TRANSPORT_RD_END_CFG_CORE_MASK_FUNCTION_SPR = 0x6
PECI_TRANSPORT_RD_END_CFG_CORE_MASK_OFFSET_SPR = 0x80


def MHzToHz(hz):
    return hz * 1000 * 1000


class CpuCtx():
    pass


class MemoryCtx():
    discovery_map = {
        CPU_ID_ICX_LCC: {
            "MC0": [(26, 0, 0x2080C),
                    (26, 0, 0x20810),
                    (26, 0, 0x2480C),
                    (26, 0, 0x24810)
                    ],
            "MC1": [(27, 0, 0x2080C),
                    (27, 0, 0x20810),
                    (27, 0, 0x2480C),
                    (27, 0, 0x24810)
                    ],
            "MC2": [(28, 0, 0x2080C),
                    (28, 0, 0x20810),
                    (28, 0, 0x2480C),
                    (28, 0, 0x24810)
                    ],
            "MC3": [(29, 0, 0x2080C),
                    (29, 0, 0x20810),
                    (29, 0, 0x2480C),
                    (29, 0, 0x24810)
                    ],
        },
        CPU_ID_ICX_HCC: {
            "MC0": [(26, 0, 0x2080C),
                    (26, 0, 0x20810),
                    (26, 0, 0x2480C),
                    (26, 0, 0x24810)
                    ],
            "MC1": [(27, 0, 0x2080C),
                    (27, 0, 0x20810),
                    (27, 0, 0x2480C),
                    (27, 0, 0x24810)
                    ],
            "MC2": [(28, 0, 0x2080C),
                    (28, 0, 0x20810),
                    (28, 0, 0x2480C),
                    (28, 0, 0x24810)
                    ],
            "MC3": [(29, 0, 0x2080C),
                    (29, 0, 0x20810),
                    (29, 0, 0x2480C),
                    (29, 0, 0x24810)
                    ],
        },
        CPU_ID_SPR: {
            "MC0": [(26, 0, 0x2080C),
                    (26, 0, 0x20810),
                    (26, 0, 0x2880C),
                    (26, 0, 0x28810)
                    ],
            "MC1": [(27, 0, 0x2080C),
                    (27, 0, 0x20810),
                    (27, 0, 0x2880C),
                    (27, 0, 0x28810)
                    ],
            "MC2": [(28, 0, 0x2080C),
                    (28, 0, 0x20810),
                    (28, 0, 0x2880C),
                    (28, 0, 0x28810)
                    ],
            "MC3": [(29, 0, 0x2080C),
                    (29, 0, 0x20810),
                    (29, 0, 0x2880C),
                    (29, 0, 0x28810)
                    ],
        },
    }


class IioCtx():
    link_status_map = {
        CPU_ID_ICX_LCC: {
            'PORT0': (1, [2, 3, 4, 5], 0x52),
            'PORT1': (2, [2, 3, 4, 5], 0x52),
            'PORT2': (4, [2, 3, 4, 5], 0x52),
            'PORT3': (5, [2, 3, 4, 5], 0x52),
            'DMI': (0, [3],          0x1B2)
        },
        CPU_ID_ICX_HCC: {
            'PORT0': (1, [2, 3, 4, 5], 0x52),
            'PORT1': (2, [2, 3, 4, 5], 0x52),
            'PORT2': (4, [2, 3, 4, 5], 0x52),
            'PORT3': (5, [2, 3, 4, 5], 0x52),
            'DMI': (0, [3],          0x1B2)
        },
        # 2,4,6,8 - PCI Gen 4 only, adjust for Gen5 when proper Gen5 CPU appears
        CPU_ID_SPR: {
            'PORT0': (0, [2, 4, 6, 8], 0x52),
            'PORT1': (1, [2, 4, 6, 8], 0x52),
            'PORT2': (2, [2, 4, 6, 8], 0x52),
            'PORT3': (3, [2, 4, 6, 8], 0x52),
            'PORT4': (4, [2, 4, 6, 8], 0x52),
            'PORT5': (5, [2, 4, 6, 8], 0x52)
        },
    }


class PeciHeader(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("address",   ctypes.c_uint8),
                ("wr_len",    ctypes.c_uint8),
                ("rd_len",    ctypes.c_uint8),
                ("command",   ctypes.c_uint8),
                ("host_id",   ctypes.c_uint8)]


class RdPkgCfg(PeciHeader):
    _pack_ = 1
    _fields_ = [("index",     ctypes.c_uint8),
                ("param",     ctypes.c_uint16)]

    def __init__(self, rd_len, param, index):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_RD_PKG_CFG_WR_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_RD_PKG_CFG
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.param = param
        self.index = index


class RdPkgCfgDevFuncOff(PeciHeader):
    _pack_ = 1
    _fields_ = [("index",    ctypes.c_uint8),
                ("device",   ctypes.c_uint16, 5),
                ("function", ctypes.c_uint16, 3),
                ("offset",   ctypes.c_uint16, 8)]

    def __init__(self, rd_len, device, function, offset, index):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_RD_PKG_CFG_WR_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_RD_PKG_CFG
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.device = device
        self.function = function
        self.offset = offset
        self.index = index


class RdPkgCfgFusedAvxCore(PeciHeader):
    _pack_ = 1
    _fields_ = [("index",      ctypes.c_uint8),
                ("fused",      ctypes.c_uint16, 1),
                ("avx_level",  ctypes.c_uint16, 4),
                ("core_count", ctypes.c_uint16, 7)]

    def __init__(self, rd_len, fused, avx_level, core_count, index):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_RD_PKG_CFG_WR_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_RD_PKG_CFG
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.fused = fused
        self.avx_level = avx_level
        self.core_count = core_count
        self.index = index


class EndpointCfg(PeciHeader):
    _pack_ = 1
    _fields_ = [("message_type", ctypes.c_uint8),
                ("eid",          ctypes.c_uint8),
                ("fid",          ctypes.c_uint8),
                ("bar",          ctypes.c_uint8),
                ("address_type", ctypes.c_uint8),
                ("segment",      ctypes.c_uint8)]


class RdEndpointCfgPciLocal(EndpointCfg):
    _pack_ = 1
    _fields_ = [("reg",          ctypes.c_uint32, 12),
                ("function",     ctypes.c_uint32, 3),
                ("device",       ctypes.c_uint32, 5),
                ("bus",          ctypes.c_uint32, 8),
                ("reserved",     ctypes.c_uint32, 4)]

    def __init__(self, rd_len, eid, fid, bar, segment,
                 bus, device, function, reg):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_RD_ENDPOINT_CFG_LOC_PCI_CFG_WR_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_RD_ENDPOINT_CFG_SPACE
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.message_type = PECI_TRANSPORT_ENDPOINT_CFG_LOC_PCI_CFG_MSG_TYPE
        self.eid = eid
        self.fid = fid
        self.bar = bar
        self.address_type = PECI_TRANSPORT_ENDPOINT_CFG_LOC_PCI_CFG_ADDR_TYPE
        self.segment = segment
        self.reg = reg
        self.function = function
        self.device = device
        self.bus = bus
        self.reserved = 0


class WrEndpointCfgPciLocal4B(EndpointCfg):
    _pack_ = 1
    _fields_ = [("reg",          ctypes.c_uint32, 12),
                ("function",     ctypes.c_uint32, 3),
                ("device",       ctypes.c_uint32, 5),
                ("bus",          ctypes.c_uint32, 8),
                ("reserved",     ctypes.c_uint32, 4),
                ("value",        ctypes.c_uint32),
                ("aw_fcs",       ctypes.c_uint8)]

    def __init__(self, rd_len, eid, fid, bar, segment,
                 bus, device, function, reg, value, aw_fcs):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_WR_ENDPOINT_CFG_LOC_PCI_4B_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_WR_ENDPOINT_CFG_LOC_PCI
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.message_type = PECI_TRANSPORT_ENDPOINT_CFG_LOC_PCI_CFG_MSG_TYPE
        self.eid = eid
        self.fid = fid
        self.bar = bar
        self.address_type = PECI_TRANSPORT_ENDPOINT_CFG_LOC_PCI_CFG_ADDR_TYPE
        self.segment = segment
        self.reg = reg
        self.function = function
        self.device = device
        self.bus = bus
        self.reserved = 0
        self.value = value
        self.aw_fcs = aw_fcs


class RdEndpointCfgMMIO32(EndpointCfg):
    _pack_ = 1
    _fields_ = [("function",     ctypes.c_uint8, 3),
                ("device",       ctypes.c_uint8, 5),
                ("bus",          ctypes.c_uint8),
                ("reg",          ctypes.c_uint32)]

    def __init__(self, rd_len, eid, fid, bar, segment,
                 bus, device, function, reg):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_RD_ENDPOINT_CFG_MMIO32_BDFBAR_WR_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_RD_ENDPOINT_CFG_SPACE
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.message_type = PECI_TRANSPORT_ENDPOINT_CFG_MMIO_BDFBAR_MSG_TYPE
        self.eid = eid
        self.fid = fid
        self.bar = bar
        self.address_type = PECI_TRANSPORT_ENDPOINT_CFG_MMIO32_BDFBAR_ADDR_TYPE
        self.segment = segment
        self.reg = reg
        self.function = function
        self.device = device
        self.bus = bus


class RdEndpointCfgMMIO64(EndpointCfg):
    _pack_ = 1
    _fields_ = [("function",     ctypes.c_uint8, 3),
                ("device",       ctypes.c_uint8, 5),
                ("bus",          ctypes.c_uint8),
                ("reg",          ctypes.c_uint64)]

    def __init__(self, rd_len, eid, fid, bar, segment,
                 bus, device, function, reg):
        self.address = PECI_TRANSPORT_CPU0_ADDRESS
        self.wr_len = PECI_TRANSPORT_RD_ENDPOINT_CFG_MMIO64_BDFBAR_WR_LEN
        self.rd_len = rd_len + 1  # Completion code
        self.command = PECI_COMMAND_RD_ENDPOINT_CFG_SPACE
        self.host_id = PECI_TRANSPORT_DEFAULT_HOST_ID
        self.message_type = PECI_TRANSPORT_ENDPOINT_CFG_MMIO_BDFBAR_MSG_TYPE
        self.eid = eid
        self.fid = fid
        self.bar = bar
        self.address_type = PECI_TRANSPORT_ENDPOINT_CFG_MMIO64_BDFBAR_ADDR_TYPE
        self.segment = segment
        self.reg = reg
        self.function = function
        self.device = device
        self.bus = bus


# Responses
class WrXppMrRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = []


class WrXppMerRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = []


class WrXpperConfRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = []


class CpuIdRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("cpu_id", ctypes.c_uint32)]


class CpuC0CounterRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("c0_counter", ctypes.c_uint64)]


class MaxTurboRatioRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("ratio", ctypes.c_uint8 * 4)]


class CpuBusNumberIcxRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("bus_number0", ctypes.c_uint8),
                ("bus_number1", ctypes.c_uint8),
                ("segment",     ctypes.c_uint8),
                ("reserved",    ctypes.c_uint8)]


class CpuBusNumberSprRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("bus_number0", ctypes.c_uint8),
                ("bus_number1", ctypes.c_uint8),
                ("segment",     ctypes.c_uint8),
                ("reserved",    ctypes.c_uint8)]


class LinkStatusIcxRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("speed",    ctypes.c_uint32, 4),
                ("width",    ctypes.c_uint32, 6),
                ("unused0",  ctypes.c_uint32, 3),
                ("active",   ctypes.c_uint32, 1),
                ("unused1",  ctypes.c_uint32, 18)]


class LinkStatusSprRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("speed",    ctypes.c_uint32, 4),
                ("width",    ctypes.c_uint32, 6),
                ("unused0",  ctypes.c_uint32, 3),
                ("active",   ctypes.c_uint32, 1),
                ("unused1",  ctypes.c_uint32, 18)]


class RdXppMrRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("cntrst",     ctypes.c_uint32, 1),
                ("ovs",        ctypes.c_uint32, 1),
                ("reserved0",  ctypes.c_uint32, 1),
                ("pto",        ctypes.c_uint32, 2),
                ("pmssig",     ctypes.c_uint32, 1),
                ("cmpmd",      ctypes.c_uint32, 2),
                ("rstevsel",   ctypes.c_uint32, 3),
                ("cens",       ctypes.c_uint32, 3),
                ("cntmd",      ctypes.c_uint32, 2),
                ("evpolinv",   ctypes.c_uint32, 1),
                ("cntevsel",   ctypes.c_uint32, 2),
                ("egs",        ctypes.c_uint32, 2),
                ("ldes",       ctypes.c_uint32, 1),
                ("dfxlnsel",   ctypes.c_uint32, 2),
                ("reserved1",  ctypes.c_uint32, 3),
                ("rstpulsen",  ctypes.c_uint32, 1),
                ("letcensel",  ctypes.c_uint32, 1),
                ("frcpmdaddz", ctypes.c_uint32, 1)]


class RdXppMerRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("tx_rx_sel",         ctypes.c_uint32, 2),
                ("fcc_sel",           ctypes.c_uint32, 3),
                ("vc_sel",            ctypes.c_uint32, 3),
                ("all_vc_sel",        ctypes.c_uint32, 1),
                ("hd_sel",            ctypes.c_uint32, 1),
                ("qm_sel",            ctypes.c_uint32, 2),
                ("reserved0",         ctypes.c_uint32, 1),
                ("link_util",         ctypes.c_uint32, 4),
                ("xp_res_assignment", ctypes.c_uint32, 4),
                ("rx_l0s_tate_util",  ctypes.c_uint32, 1),
                ("tx_l0s_tate_util",  ctypes.c_uint32, 1),
                ("count_ce",          ctypes.c_uint32, 1),
                ("count_ue",          ctypes.c_uint32, 1),
                ("l1_state_util",     ctypes.c_uint32, 1)]


class RdXpperConfRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("global_reset",      ctypes.c_uint8, 1),
                ("global_counter_en", ctypes.c_uint8, 1)]


class RdXppMdlRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("counter", ctypes.c_uint32)]


class RdXppMdhRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("counter0",  ctypes.c_uint32, 4),
                ("reserved0", ctypes.c_uint32, 4),
                ("counter1",  ctypes.c_uint32, 4),
                ("reserved1", ctypes.c_uint32, 4)]


class RdDluCntrRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("link_util_cntr0",  ctypes.c_uint32, 8),
                ("link_util_cntr1",  ctypes.c_uint32, 8),
                ("link_util_cntr2",  ctypes.c_uint32, 8),
                ("link_util_cntr3",  ctypes.c_uint32, 8)]


class CapabilityRegisterRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("unused0",     ctypes.c_uint32, 26),
                ("eet_enable",  ctypes.c_uint32, 1)]


class CoreCountLowRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("core_count", ctypes.c_uint32)]


class CoreCountHighRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("core_count", ctypes.c_uint32)]


class MemoryFrequencyRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("frequency", ctypes.c_uint32, 6),
                ("unused", ctypes.c_uint32, 2),
                ("type", ctypes.c_uint32, 4),
                ("unused", ctypes.c_uint32, 20)]


class MaxNonTurboRatioRsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("unused0", ctypes.c_uint8),
                ("ratio",   ctypes.c_uint8),
                ("unused1", ctypes.c_uint8),
                ("unused2", ctypes.c_uint8)]


class MemoryDimmmtr32Rsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("ca_width",            ctypes.c_uint32, 2),
                ("ra_width",            ctypes.c_uint32, 3),
                ("ddr3_dnsty",          ctypes.c_uint32, 3),
                ("ddr3_width",          ctypes.c_uint32, 2),
                ("ba_shrink",           ctypes.c_uint32, 1),
                ("unused0",             ctypes.c_uint32, 1),
                ("rank_cnt",            ctypes.c_uint32, 2),
                ("unused1",             ctypes.c_uint32, 1),
                ("dimm_pop",            ctypes.c_uint32, 1),
                ("rank_disable",        ctypes.c_uint32, 4),
                ("unused2",             ctypes.c_uint32, 1),
                ("ddr4_mode",           ctypes.c_uint32, 1),
                ("hdrl",                ctypes.c_uint32, 1),
                ("hdrl_parity",         ctypes.c_uint32, 1),
                ("ddr4_3dsnumranks_cs", ctypes.c_uint32, 2)]


class MemoryDimmmtr64Rsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("ca_width",            ctypes.c_uint32, 2),
                ("ra_width",            ctypes.c_uint32, 3),
                ("ddr3_dnsty",          ctypes.c_uint32, 3),
                ("ddr3_width",          ctypes.c_uint32, 2),
                ("ba_shrink",           ctypes.c_uint32, 1),
                ("unused0",             ctypes.c_uint32, 1),
                ("rank_cnt",            ctypes.c_uint32, 2),
                ("unused1",             ctypes.c_uint32, 1),
                ("dimm_pop",            ctypes.c_uint32, 1),
                ("rank_disable",        ctypes.c_uint32, 4),
                ("unused2",             ctypes.c_uint32, 1),
                ("ddr4_mode",           ctypes.c_uint32, 1),
                ("hdrl",                ctypes.c_uint32, 1),
                ("hdrl_parity",         ctypes.c_uint32, 1),
                ("ddr4_3dsnumranks_cs", ctypes.c_uint32, 2)]


class MemoryRdCounter32Rsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("counter", ctypes.c_uint64)]


class MemoryWrCounter32Rsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("counter", ctypes.c_uint64)]


class MemoryRdCounter64Rsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("counter", ctypes.c_uint64)]


class MemoryWrCounter64Rsp(ctypes.Structure):
    _pack_ = 1
    _fields_ = [("counter", ctypes.c_uint64)]


peci_commands = {
    "CpuId": RdPkgCfg(
        rd_len=ctypes.sizeof(CpuIdRsp),
        param=0,
        index=0
    ),
    "CpuC0Counter": RdPkgCfg(
        rd_len=ctypes.sizeof(CpuC0CounterRsp),
        param=0xFE,
        index=31
    ),
    "MaxTurboRatio": RdPkgCfgFusedAvxCore(
        rd_len=ctypes.sizeof(MaxTurboRatioRsp),
        fused=1,
        avx_level=0,
        core_count=0,
        index=49
    ),
    "CpuBusNumberIcx": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(CpuBusNumberIcxRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=13,
        device=0,
        function=2,
        reg=0
    ),
    "CpuBusNumberSpr": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(CpuBusNumberSprRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=30,
        device=0,
        function=2,
        reg=0xD0
    ),
    "LinkStatusIcx": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(LinkStatusIcxRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0
    ),
    "LinkStatusSpr": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(LinkStatusSprRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0
    ),
    "RdXppMr": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(RdXppMrRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x594,
    ),
    "RdXppMer": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(RdXppMerRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x5AC,
    ),
    "RdXpperConf": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(RdXpperConfRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x5C4,
    ),
    "WrXppMr": WrEndpointCfgPciLocal4B(
        rd_len=ctypes.sizeof(WrXppMrRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x594,
        value=0x100800,
        aw_fcs=0
    ),
    "WrXppMer": WrEndpointCfgPciLocal4B(
        rd_len=ctypes.sizeof(WrXppMerRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x5AC,
        value=0x1e011f,
        aw_fcs=0
    ),
    "WrXpperConf": WrEndpointCfgPciLocal4B(
        rd_len=ctypes.sizeof(WrXpperConfRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x5C4,
        value=0x2,
        aw_fcs=0
    ),
    "RdXppMdl": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(RdXppMdlRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x580,
    ),
    "RdXppMdh": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(RdXppMdhRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=2,
        function=0,
        reg=0x590,
    ),
    "RdDluCntr": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(RdDluCntrRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=0,
        device=3,
        function=0,
        reg=0x4C0,
    ),
    "CapabilityRegister": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(CapabilityRegisterRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=31,
        device=30,
        function=3,
        reg=0x94
    ),
    "CoreCountLow": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(CoreCountLowRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=31,
        device=30,
        function=3,
        reg=0xD0
    ),
    "CoreCountHigh": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(CoreCountHighRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=31,
        device=30,
        function=3,
        reg=0xD4
    ),
    "MemoryFrequency": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(MemoryFrequencyRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=31,
        device=30,
        function=1,
        reg=0x98
    ),
    "MaxNonTurboRatio": RdEndpointCfgPciLocal(
        rd_len=ctypes.sizeof(MaxNonTurboRatioRsp),
        eid=0,
        fid=0,
        bar=0,
        segment=0,
        bus=31,
        device=30,
        function=0,
        reg=0xA8
    ),
    "MemoryDimmmtr32": RdEndpointCfgMMIO32(
        rd_len=ctypes.sizeof(MemoryDimmmtr32Rsp),
        eid=0,
        fid=0,
        bar=PECI_MEMORY_CONTROLLER_BAR,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0
    ),
    "MemoryDimmmtr64": RdEndpointCfgMMIO64(
        rd_len=ctypes.sizeof(MemoryDimmmtr64Rsp),
        eid=0,
        fid=0,
        bar=PECI_MEMORY_CONTROLLER_BAR,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0
    ),
    "MemoryRdCounter32": RdEndpointCfgMMIO32(
        rd_len=ctypes.sizeof(MemoryRdCounter32Rsp),
        eid=0,
        fid=0,
        bar=PECI_MEMORY_CONTROLLER_BAR,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0x2290
    ),
    "MemoryWrCounter32": RdEndpointCfgMMIO32(
        rd_len=ctypes.sizeof(MemoryWrCounter32Rsp),
        eid=0,
        fid=0,
        bar=PECI_MEMORY_CONTROLLER_BAR,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0x2298
    ),
    "MemoryRdCounter64": RdEndpointCfgMMIO64(
        rd_len=ctypes.sizeof(MemoryRdCounter64Rsp),
        eid=0,
        fid=0,
        bar=PECI_MEMORY_CONTROLLER_BAR,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0x2290
    ),
    "MemoryWrCounter64": RdEndpointCfgMMIO64(
        rd_len=ctypes.sizeof(MemoryWrCounter64Rsp),
        eid=0,
        fid=0,
        bar=PECI_MEMORY_CONTROLLER_BAR,
        segment=0,
        bus=0,
        device=0,
        function=0,
        reg=0x2298
    ),
}


def parse_args():
    global hostname, cpu_monitor, memory_monitor, iio_monitor, verbose

    parser = argparse.ArgumentParser()
    parser.add_argument('--host', metavar='<IP address>', required=True,
                        help='hostname or IP address of BMC')
    parser.add_argument('--cpu_monitor', '-c', action='store_true',
                        help='run CPU utilization monitor')
    parser.add_argument('--memory_monitor', '-m', action='store_true',
                        help='run memory utilization monitor')
    parser.add_argument('--iio_monitor', '-i', action='store_true',
                        help='run IIO utilization monitor')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='increases output verbosity')
    hostname = parser.parse_args().host
    cpu_monitor = parser.parse_args().cpu_monitor
    memory_monitor = parser.parse_args().memory_monitor
    iio_monitor = parser.parse_args().iio_monitor
    verbose = parser.parse_args().verbose


def send_peci_cmd(cmd):
    (stdin, stdout, stderr) = ssh_connection.exec_command("peci_cmds raw {cmd}"
                                                          .format(cmd=cmd))
    return stdout.read()


def create_request_string(request):
    request_string = ''.join(["0x%02x " % byte for byte in request])
    return request_string


def create_request_bytes(cmd):
    request_size = cmd.wr_len + PECI_HEADER_LEN
    request_bytes = bytes(cmd)
    cmd_size = len(request_bytes)

    if (request_size < cmd_size):
        # Structure has additional padding that should not be
        # included as a part of request
        if verbose:
            print('ERROR: Command truncated (request size: {} < buffer size: {}'
                  .format(request_size, cmd_size))
            exit(-1)

    elif (request_size > cmd_size):
        print('ERROR: Invalid request size: {} > buffer size: {}'
              .format(request_size, cmd_size))
        exit(-1)
        return None

    return request_bytes


def peci_send_request(cmd):
    # Create request (bytes) from command
    request_bytes = create_request_bytes(cmd)
    if (request_bytes == None):
        return None

    # Create a string based on previously prepared request (bytes)
    request_string = create_request_string(request_bytes)
    if verbose:
        print('Request: {}'.format(request_string))

    # Send request and return the response
    response = send_peci_cmd(request_string).strip().split()
    comp_code = int(response[0], 0)
    if (comp_code != 0x40):
        print('Invalid completion code: {}'.format(comp_code))
        return None

    # Convert response to bytearray with integers
    resp_bytes = bytearray([int(x, 0) for x in response[1:]])

    return resp_bytes


def peci_cmd_execute(cmd_name):
    if verbose:
        print(f"== Sending command: {cmd_name} ==")

    cmd = peci_commands[cmd_name]
    rsp = globals()[cmd_name + 'Rsp'].from_buffer_copy(peci_send_request(cmd))

    if verbose:
        print('Response:  {:>#x}'.format(int.from_bytes(bytes(rsp),
                                                        byteorder='little',
                                                        signed=False)))

    return rsp


def cpu_is_turbo_enabled():
    rsp = peci_cmd_execute('CapabilityRegister')

    return rsp.eet_enable == 1


def cpu_get_mask(cpu_id):
    if (cpu_id == CPU_ID_ICX_LCC):
        function = PECI_TRANSPORT_RD_END_CFG_CORE_MASK_FUNCTION_ICX
        offset = PECI_TRANSPORT_RD_END_CFG_CORE_MASK_OFFSET_ICX
    elif (cpu_id == CPU_ID_ICX_HCC):
        function = PECI_TRANSPORT_RD_END_CFG_CORE_MASK_FUNCTION_ICX
        offset = PECI_TRANSPORT_RD_END_CFG_CORE_MASK_OFFSET_ICX
    elif (cpu_id == CPU_ID_SPR):
        function = PECI_TRANSPORT_RD_END_CFG_CORE_MASK_FUNCTION_SPR
        offset = PECI_TRANSPORT_RD_END_CFG_CORE_MASK_OFFSET_SPR
    else:
        print('Invalid CPU ID')
        return None

    peci_commands['CoreCountLow'].function = function
    peci_commands['CoreCountLow'].reg = offset
    peci_commands['CoreCountHigh'].function = function
    peci_commands['CoreCountHigh'].reg = offset + 0x04

    rsp = peci_cmd_execute('CoreCountLow')
    cpu_mask = rsp.core_count

    rsp = peci_cmd_execute('CoreCountHigh')
    cpu_mask |= rsp.core_count << 32

    return cpu_mask


def cpu_get_max_non_turbo_ratio():
    rsp = peci_cmd_execute('MaxNonTurboRatio')

    return rsp.ratio


def cpu_get_max_turbo_ratio(cpu_count):
    calc_cpu_count = cpu_count
    if ((cpu_count % CORES_IN_GET_TURBO_RESPONSE) == 0):
        calc_cpu_count -= 1
    calc_cpu_count //= CORES_IN_GET_TURBO_RESPONSE
    calc_cpu_idx = cpu_count - \
        (CORES_IN_GET_TURBO_RESPONSE * calc_cpu_count) - 1

    peci_commands['MaxTurboRatio'].core_count = calc_cpu_count
    rsp = peci_cmd_execute('MaxTurboRatio')

    return rsp.ratio[calc_cpu_idx]


def cpu_get_id():
    rsp = peci_cmd_execute('CpuId')

    return rsp.cpu_id


def cpu_get_c0_counter():
    rsp = peci_cmd_execute('CpuC0Counter')

    return rsp.c0_counter


def cpu_get_bus_number(cpu_id):
    if (cpu_id == CPU_ID_SPR):
        rsp = peci_cmd_execute('CpuBusNumberSpr')
    elif (cpu_id == CPU_ID_ICX_HCC):
        peci_commands['CpuBusNumberIcx'].reg = PECI_TRANSPORT_RD_PCI_CFG_BUS_NUMBER_OFFSET_ICX_HCC
        rsp = peci_cmd_execute('CpuBusNumberIcx')
    elif (cpu_id == CPU_ID_ICX_LCC):
        peci_commands['CpuBusNumberIcx'].reg = PECI_TRANSPORT_RD_PCI_CFG_BUS_NUMBER_OFFSET_ICX_LCC
        rsp = peci_cmd_execute('CpuBusNumberIcx')
    else:
        print('Invalid CPU ID')
        return None

    return rsp.bus_number0


def cpu_calc_count_from_mask(mask):
    cpu_count = 0

    while mask > 0:
        if (mask & 0x1):
            cpu_count += 1
        mask >>= 1

    return cpu_count


def cpu_discovery(cpu):
    print('CPU discovery:')

    cpu.id = cpu_get_id()
    cpu.mask = cpu_get_mask(cpu.id)
    cpu.count = cpu_calc_count_from_mask(cpu.mask)
    cpu.non_turbo_freq = cpu_get_max_non_turbo_ratio() * 100
    cpu.turbo_freq = cpu_get_max_turbo_ratio(cpu.count) * 100
    cpu.bus_number = cpu_get_bus_number(cpu.id)
    cpu.turbo = cpu_is_turbo_enabled()
    if cpu.turbo:
        cpu.max_util = cpu.count * MHzToHz(cpu.turbo_freq)
    else:
        cpu.max_util = cpu_count * MHzToHz(cpu.non_turbo_cpu_freq)

    turbo_string = 'Enabled' if cpu.turbo else 'Disabled'

    print('\tCPU ID:                   {:>#16x}'.format(cpu.id))
    print('\tCPU mask:                 {:>#16x}'.format(cpu.mask))
    print('\tCPU count:                {:>#16}' .format(cpu.count))
    print('\tCPU freq (non-turbo MHz): {:>#16}' .format(cpu.non_turbo_freq))
    print('\tCPU freq (turbo MHz):     {:>#16}' .format(cpu.turbo_freq))
    print('\tCPU Max utilization:      {:>16}'  .format(cpu.max_util))
    print('\tTurbo:                    {:>16}'  .format(turbo_string))
    print('\tCPU Bus number:           {:>#16x}'.format(cpu.bus_number))


def memory_get_frequency():
    rsp = peci_cmd_execute('MemoryFrequency')

    # Per PCU_CR_MC_BIOS_REQ_CFG specification
    if rsp.type == 0:
        return rsp.frequency * 133.33
    elif rsp.type == 1:
        return rsp.frequency * 100
    else:
        print("Unexpected REQ_TYPE")
        return None


def memory_get_dimmmtr(cpu_id, bus, device, function, offset):
    if cpu_id == CPU_ID_SPR:
        command = 'MemoryDimmmtr64'
    else:
        command = 'MemoryDimmmtr32'

    peci_commands[command].bus = bus
    peci_commands[command].device = device
    peci_commands[command].function = function
    peci_commands[command].reg = offset
    rsp = peci_cmd_execute(command)

    return rsp


def memory_get_rd_counter(cpu_id, bus, device):
    if cpu_id == CPU_ID_SPR:
        command = 'MemoryRdCounter64'
    else:
        command = 'MemoryRdCounter32'

    peci_commands[command].bus = bus
    peci_commands[command].device = device
    rsp = peci_cmd_execute(command)

    return rsp.counter


def memory_get_wr_counter(cpu_id, bus, device):
    if cpu_id == CPU_ID_SPR:
        command = 'MemoryWrCounter64'
    else:
        command = 'MemoryWrCounter32'

    peci_commands[command].bus = bus
    peci_commands[command].device = device
    rsp = peci_cmd_execute(command)

    return rsp.counter


def memory_discovery(memory):
    print('Memory discovery:')

    # Discover memory channels
    memory.channels = 0
    for mcx in memory.discovery_map[memory.cpu_id]:
        print('\t{}'.format(mcx))

        for (device, function, offset) in memory.discovery_map[memory.cpu_id][mcx]:
            rsp = memory_get_dimmmtr(
                memory.cpu_id, memory.bus_number, device, function, offset)
            if rsp.dimm_pop:
                memory.channels += 1

            print('\t\tDevice: {} Function: {} Offset:{:>#8x} DIMMMTR: {:>#6x}'
                  .format(device, function, offset,
                          int.from_bytes(bytes(rsp), byteorder='little',
                                         signed=False)))

    memory.frequency = memory_get_frequency()
    memory.max_util = int((memory.channels * MHzToHz(memory.frequency) * 8) /
                          64)

    print('\tFrequency (MHz): {:>16.2f}'.format(memory.frequency))
    print('\tChannels:        {:>16}'   .format(memory.channels))
    print('\tMax utilization: {:>16}'   .format(memory.max_util))


def iio_get_link_status(cpu_id, bus, device, reg):
    if cpu_id == CPU_ID_SPR:
        command = 'LinkStatusSpr'
    else:
        command = 'LinkStatusIcx'

    peci_commands[command].bus = bus
    peci_commands[command].device = device
    peci_commands[command].reg = reg
    rsp = peci_cmd_execute(command)

    return rsp


def iio_get_link_speed(speed_raw):
    if (speed_raw == 1):
        return 2.5
    elif (speed_raw == 2):
        return 5
    elif (speed_raw == 3):
        return 8
    else:
        return 0


def iio_get_xppmr(port):
    peci_commands['RdXppMr'].bus = port
    rsp = peci_cmd_execute('RdXppMr')

    return rsp


def iio_get_xppmer(port):
    peci_commands['RdXppMer'].bus = port
    rsp = peci_cmd_execute('RdXppMer')

    return rsp


def iio_get_xpperconf(port):
    peci_commands['RdXpperConf'].bus = port
    rsp = peci_cmd_execute('RdXpperConf')

    return rsp


def iio_set_xppmr(port, value):
    peci_commands['WrXppMr'].bus = port
    peci_commands['WrXppMr'].value = value
    rsp = peci_cmd_execute('WrXppMr')

    return rsp


def iio_set_xppmer(port, value):
    peci_commands['WrXppMer'].bus = port
    peci_commands['WrXppMer'].value = value
    rsp = peci_cmd_execute('WrXppMer')

    return rsp


def iio_set_xpperconf(port, value):
    peci_commands['WrXpperConf'].bus = port
    peci_commands['WrXpperConf'].value = value
    rsp = peci_cmd_execute('WrXpperConf')

    return rsp


def iio_get_gen3_counter(port):
    peci_commands['RdDluCntr'].bus = port
    rsp = peci_cmd_execute('RdDluCntr')
    counter = rsp.link_util_cntr0
    counter += rsp.link_util_cntr1
    counter += rsp.link_util_cntr2
    counter += rsp.link_util_cntr3

    return counter


def iio_get_gen4_counter(port):
    peci_commands['RdXppMdl'].bus = port
    rsp = peci_cmd_execute('RdXppMdl')
    counter = rsp.counter

    peci_commands['RdXppMdh'].bus = port
    rsp = peci_cmd_execute('RdXppMdh')
    counter |= (rsp.counter0 << 32)

    return counter


def iio_discovery(iio):
    print('IIO discovery:')

    # Discover link status
    iio.active_links = 0
    iio.max_util = 0
    for port_name in iio.link_status_map[iio.cpu_id]:
        print('\t{}'.format(port_name))

        (port, links, reg) = iio.link_status_map[iio.cpu_id][port_name]
        for link in links:
            rsp = iio_get_link_status(
                iio.cpu_id, bus=port, device=link, reg=reg)
            if rsp.active:
                iio.active_links += 1
                iio.max_util += iio_get_link_speed(rsp.speed) * \
                    rsp.width * FULL_DUPLEX

            print('\t\tLink: {} Active: {} Speed (raw): {:>#2x} Speed (Gbps): {:>4} Width: {:>#4x} '
                  .format(link, rsp.active, rsp.speed,
                          iio_get_link_speed(rsp.speed), rsp.width))

        if (port_name != 'DMI'):
            xppmr = iio_get_xppmr(port)
            xppmer = iio_get_xppmer(port)
            xpperconf = iio_get_xpperconf(port)

            raw_xppmr = int.from_bytes(
                bytes(xppmr), byteorder='little', signed=False)
            raw_xppmer = int.from_bytes(
                bytes(xppmer), byteorder='little', signed=False)
            raw_xpperconf = int.from_bytes(
                bytes(xpperconf), byteorder='little', signed=False)

            print('\t\tXPPMR: {:>#15x}\n\t\tXPPMER: {:>#14x}\n\t\tXPPERCONF: {:>#11x}'
                  .format(raw_xppmr, raw_xppmer, raw_xpperconf))

            if raw_xppmr != EXPECTED_XPP_MR:
                print("Updating XPP_MR")
                iio_set_xppmr(port, EXPECTED_XPP_MR)
            if raw_xppmer != EXPECTED_XPP_MER:
                print("Updating XPP_MER")
                iio_set_xppmer(port, EXPECTED_XPP_MER)
            if raw_xpperconf != EXPECTED_XPPER_CONF:
                print("Updating XPPER_CONF")
                iio_set_xpperconf(port, EXPECTED_XPPER_CONF)

    # Convert to bytes per second
    iio.max_util = (iio.max_util * DWORDS_IN_GIGABYTE) / 8

    print('\tActive links:          {}'.format(iio.active_links))
    print('\tMax utilization (B/s): {}'.format(iio.max_util))


def run_cpu_monitor(cpu):
    interval = 2
    start_time = 0
    prev_value = 0
    ready = False

    while True:
        elapsed = time.perf_counter() - start_time
        start_time = time.perf_counter()

        new_value = cpu_get_c0_counter()
        delta = new_value - prev_value
        prev_value = new_value

        # Skip utilization calculation if it was first reading
        if ready:
            util = delta / (cpu.max_util * elapsed)
            # If command execution time was too long, counter value can be too high
            if (util > 1):
                util = 1
            print('CPU utilization: {:06.2%} interval: {:.2f}s'.format(
                util, elapsed))

        ready = True
        code_execution_time = time.perf_counter() - start_time
        if (code_execution_time < interval):
            time.sleep(interval - code_execution_time)


def run_memory_monitor(memory):
    interval = 9
    start_time = 0
    ready = False
    tmp = {mcx: {'prev_value': 0,
                 'delta': 0} for mcx in memory.discovery_map[memory.cpu_id]}

    while True:
        delta = 0
        elapsed = time.perf_counter() - start_time
        start_time = time.perf_counter()

        for mcx in memory.discovery_map[memory.cpu_id]:
            # There are two counters (rd and wr) for each MCx
            (device, function,
             offset) = memory.discovery_map[memory.cpu_id][mcx][0]
            new_value = memory_get_rd_counter(memory.cpu_id, memory.bus_number,
                                              device)
            new_value += memory_get_wr_counter(memory.cpu_id, memory.bus_number,
                                               device)
            tmp[mcx]['delta'] = new_value - tmp[mcx]['prev_value']
            tmp[mcx]['prev_value'] = new_value
            delta += tmp[mcx]['delta']

        # Skip utilization calculation if it was first reading
        if ready:
            util = delta / (memory.max_util * elapsed)
            # If command execution time was too long, counter value can be too high
            if (util > 1):
                util = 1
            print('Memory utilization: {:06.2%} interval: {:.2f}s'.format(
                util, elapsed))

        ready = True
        code_execution_time = time.perf_counter() - start_time
        if (code_execution_time < interval):
            time.sleep(interval - code_execution_time)


def run_iio_monitor(iio):
    interval = 9
    start_time = 0
    ready = False
    tmp = {port: {'prev_value': 0,
                  'delta': 0} for port in iio.link_status_map[iio.cpu_id]}

    while True:
        delta = 0
        elapsed = time.perf_counter() - start_time
        start_time = time.perf_counter()

        for port in iio.link_status_map[iio.cpu_id]:
            (device, links, reg) = iio.link_status_map[iio.cpu_id][port]
            if (port == 'DMI'):
                # Gen3 counter returns value in GBs
                new_value = iio_get_gen3_counter(device) * DWORDS_IN_GIGABYTE
            else:
                # Gen4 counter returns value in DWORDs (4B)
                new_value = iio_get_gen4_counter(device)

            # Handle oveflow
            if (new_value < tmp[port]['prev_value']):
                print('IIO overflow detected')
                tmp[port]['delta'] = (
                    IIO_OVERFLOW - tmp[port]['prev_value']) + new_value
            else:
                tmp[port]['delta'] = new_value - tmp[port]['prev_value']

            tmp[port]['prev_value'] = new_value
            delta += tmp[port]['delta']

        # Skip utilization calculation if it was first reading
        if ready:
            util = delta / (iio.max_util * elapsed)
            # If command execution time was too long, counter value can be too high
            if (util > 1):
                util = 1
            print('IIO utilization: {:06.2%} interval: {:.2f}s'.format(
                util, elapsed))

        ready = True
        code_execution_time = time.perf_counter() - start_time
        if (code_execution_time < interval):
            time.sleep(interval - code_execution_time)


def cups():
    global ssh_connection

    # Parse input parameters
    parse_args()

    # Configure SSH connection
    ssh_connection = paramiko.SSHClient()
    ssh_connection.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_connection.connect(hostname, username='root')

    # CPU
    cpu_ctx = CpuCtx()
    cpu_discovery(cpu_ctx)

    # Memory
    memory_ctx = MemoryCtx()
    memory_ctx.cpu_id = cpu_ctx.id
    memory_ctx.bus_number = cpu_ctx.bus_number
    memory_discovery(memory_ctx)

    # # IIO
    iio_ctx = IioCtx()
    iio_ctx.cpu_id = cpu_ctx.id
    iio_discovery(iio_ctx)

    if cpu_monitor:
        run_cpu_monitor(cpu_ctx)
    if memory_monitor:
        run_memory_monitor(memory_ctx)
    if iio_monitor:
        run_iio_monitor(iio_ctx)

    # Close SSH connection
    ssh_connection.close()


if __name__ == "__main__":
    cups()
