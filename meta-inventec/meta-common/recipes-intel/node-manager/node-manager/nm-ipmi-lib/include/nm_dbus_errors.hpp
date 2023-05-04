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
#include <systemd/sd-bus.h>

#include <type_traits>

namespace nmipmi
{

// IPMI NM completion codes specified by the Intel OpenBMC Node Manager External
// Interface Specification
//
enum class ErrorCodes : int
{
    BadReadFcsIn = 1000,
    BadWriteFcsField,
    CmdNotSupported,
    CmdResponseTimeout,
    CorrectionTimeOutOfRange,
    CpuNotPresent,
    DcmiModeIsNotPresent,
    HwpAreEnabledInBios,
    IncorrectActiveCoresConfiguration,
    IncorrectKCoefficientValue,
    InsufficientPrivilegeLevel,
    InvalidComponentId,
    InvalidDomainId,
    InvalidMode,
    InvalidPolicyId,
    InvalidPolicyComponentId,
    InvalidValue,
    NoPolicyIsCurrentlyLimiting,
    OtherErrorEncountered,
    ParameterOutOfRange,
    PlatformNotInS0NorS1State,
    PoliciesCannotBeCreated,
    PolicyIdAlreadyExists,
    TriggerValueOutOfRange,
    PowerDomainNotSupported,
    PowerLimitNotSet,
    PowerLimitOutOfRange,
    PstateOutOfRange,
    ReadingSourceUnavailable,
    StatRepPeriodOutOfRange,
    TurboRatioLimitOutOfRange,
    UnsupportedPolicyTriggerType,
    UnknownPolicyType,
    WrongValueOfCpuSockets,
    InvalidArgument,
    InvalidPolicyStorage,
    InvalidLimitException,
    InvalidPowerCorrectionType,
    InvalidLogLevel,
    OutOfRange,
    OperationNotPermitted,
};

static constexpr sd_bus_error_map kNodeManagerDBusErrors[] = {
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.BadReadFcsIn",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::BadReadFcsIn)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.BadWriteFcsField",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::BadWriteFcsField)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.CmdNotSupported",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::CmdNotSupported)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.CmdResponseTimeout",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::CmdResponseTimeout)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.CorrectionTimeOutOfRange",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::CorrectionTimeOutOfRange)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.CpuNotPresent",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::CpuNotPresent)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.DcmiModeIsNotPresent",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::DcmiModeIsNotPresent)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.HwpAreEnabledInBios",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::HwpAreEnabledInBios)),
    SD_BUS_ERROR_MAP("xyz.openbmc_project.NodeManager.Error."
                     "IncorrectActiveCoresConfiguration",
                     std::underlying_type_t<ErrorCodes>(
                         ErrorCodes::IncorrectActiveCoresConfiguration)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.IncorrectKCoefficientValue",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::IncorrectKCoefficientValue)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InsufficientPrivilegeLevel",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InsufficientPrivilegeLevel)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidComponentId",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidComponentId)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidDomainId",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidDomainId)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidMode",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidMode)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidPolicyId",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidPolicyId)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidValue",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidValue)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.NoPolicyIsCurrentlyLimiting",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::NoPolicyIsCurrentlyLimiting)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.OtherErrorEncountered",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::OtherErrorEncountered)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.ParameterOutOfRange",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::ParameterOutOfRange)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PlatformNotInS0NorS1State",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PlatformNotInS0NorS1State)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PoliciesCannotBeCreated",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PoliciesCannotBeCreated)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PolicyIdAlreadyExists",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::PolicyIdAlreadyExists)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.TriggerValueOutOfRange",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::TriggerValueOutOfRange)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PowerDomainNotSupported",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PowerDomainNotSupported)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PowerLimitNotSet",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::PowerLimitNotSet)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PowerLimitOutOfRange",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::PowerLimitOutOfRange)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.PstateOutOfRange",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::PstateOutOfRange)),
    SD_BUS_ERROR_MAP("xyz.openbmc_project.NodeManager.Error."
                     "ReadingSourceUnavailable",
                     std::underlying_type_t<ErrorCodes>(
                         ErrorCodes::ReadingSourceUnavailable)),
    SD_BUS_ERROR_MAP("xyz.openbmc_project.NodeManager.Error."
                     "StatRepPeriodOutOfRange",
                     std::underlying_type_t<ErrorCodes>(
                         ErrorCodes::StatRepPeriodOutOfRange)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.TurboRatioLimitOutOfRange",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::TurboRatioLimitOutOfRange)),
    SD_BUS_ERROR_MAP("xyz.openbmc_project.NodeManager.Error."
                     "UnsupportedPolicyTriggerType",
                     std::underlying_type_t<ErrorCodes>(
                         ErrorCodes::UnsupportedPolicyTriggerType)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.UnknownPolicyType",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::UnknownPolicyType)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.WrongValueOfCpuSockets",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::WrongValueOfCpuSockets)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.Common.Error.InvalidArgument",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidArgument)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidPolicyStorage",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidPolicyStorage)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidLimitException",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidLimitException)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidPowerCorrectionType",
        std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InvalidPowerCorrectionType)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.InvalidLogLevel",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidLogLevel)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.OutOfRange",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::OutOfRange)),
    SD_BUS_ERROR_MAP(
        "xyz.openbmc_project.NodeManager.Error.OperationNotPermitted",
        std::underlying_type_t<ErrorCodes>(ErrorCodes::OperationNotPermitted)),
    SD_BUS_ERROR_MAP_END};

} // namespace nmipmi