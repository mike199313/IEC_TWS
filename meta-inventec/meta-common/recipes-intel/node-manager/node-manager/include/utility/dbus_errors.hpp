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

#include "common_types.hpp"

#include <memory>
#include <sdbusplus/exception.hpp>

namespace nodemanager
{

// When you define a new dbus exception here do not forget to add proper mapping
// to kNodeManagerDBusErrors
namespace errors
{

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

struct InvalidArgument final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Common.Error.InvalidArgument";
    static constexpr auto errDesc = "Invalid argument was given";
    static constexpr auto errWhat =
        "xyz.openbmc_project.Common.Error.InvalidArgument: Invalid argument "
        "was given";

    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidArgument);
    }
};

struct BadReadFcsIn final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.BadReadFcsIn";
    static constexpr auto errDesc = "Bad read FCS in the response";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.BadReadFcsIn: Bad "
        "read FCS in the response";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::BadReadFcsIn);
    }
};
struct BadWriteFcsField final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.BadWriteFcsField";
    static constexpr auto errDesc = "Bad write FCS field in the response";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.BadWriteFcsField: "
        "Bad write FCS field in the response";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::BadWriteFcsField);
    }
};
struct CmdNotSupported final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.CmdNotSupported";
    static constexpr auto errDesc =
        "Command not supported in the current configuration. Returned when PSU "
        "is configured as the power reading source";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.CmdNotSupported: Command "
        "not supported in the current configuration. Returned when PSU is "
        "configured as the power reading source";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::CmdNotSupported);
    }
};
struct CmdResponseTimeout final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.CmdResponseTimeout";
    static constexpr auto errDesc = "Command response timeout";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.CmdResponseTimeout: Command "
        "response timeout";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::CmdResponseTimeout);
    }
};
struct CorrectionTimeOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.CorrectionTimeOutOfRange";
    static constexpr auto errDesc = "Correction Time out of range";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.CorrectionTimeOutOfRange: "
        "Correction Time out of range";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::CorrectionTimeOutOfRange);
    }
};
struct CpuNotPresent final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.CpuNotPresent";
    static constexpr auto errDesc = "CPU not present";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.CpuNotPresent: CPU not present";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::CpuNotPresent);
    }
};
struct DcmiModeIsNotPresent final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.DcmiModeIsNotPresent";
    static constexpr auto errDesc = "Returned when DCMI mode is not present";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.DcmiModeIsNotPresent: Returned "
        "when DCMI mode is not present";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::DcmiModeIsNotPresent);
    }
};
struct HwpAreEnabledInBios final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.HwpAreEnabledInBios";
    static constexpr auto errDesc =
        "Hardware Controlled Performance States (HWP) are enabled in BIOS. "
        "Legacy ACPI Pstate setting is not supported";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.HwpAreEnabledInBios: Hardware "
        "Controlled Performance States (HWP) are enabled in BIOS. Legacy ACPI "
        "Pstate setting is not supported";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::HwpAreEnabledInBios);
    }
};
struct IncorrectActiveCoresConfiguration final : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.NodeManager.Error."
                                    "IncorrectActiveCoresConfiguration";
    static constexpr auto errDesc = "Unsupported number of active cores";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error."
        "IncorrectActiveCoresConfiguration: Unsupported number of active cores";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::IncorrectActiveCoresConfiguration);
    }
};
struct IncorrectKCoefficientValue final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.IncorrectKCoefficientValue";
    static constexpr auto errDesc = "Incorrect K Coefficient value";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.IncorrectKCoefficientValue: "
        "Incorrect K Coefficient value";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::IncorrectKCoefficientValue);
    }
};
struct InsufficientPrivilegeLevel final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InsufficientPrivilegeLevel";
    static constexpr auto errDesc =
        "Insufficient privilege level due wrong LUN";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InsufficientPrivilegeLevel: "
        "Insufficient privilege level due wrong LUN";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InsufficientPrivilegeLevel);
    }
};
struct InvalidComponentId final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidComponentId";
    static constexpr auto errDesc = "Invalid Component Identifier";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidComponentId: "
        "Invalid Component Identifier";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InvalidComponentId);
    }
};
struct InvalidDomainId final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidDomainId";
    static constexpr auto errDesc = "Invalid Domain Id";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "InvalidDomainId: Invalid Domain Id";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidDomainId);
    }
};
struct InvalidMode final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidMode";
    static constexpr auto errDesc = "Invalid Mode";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidMode: Invalid Mode";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidMode);
    }
};
struct InvalidPolicyId final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidPolicyId";
    static constexpr auto errDesc = "Invalid Policy Id";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "InvalidPolicyId: Invalid Policy Id";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidPolicyId);
    }
};
struct InvalidValue final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidValue";
    static constexpr auto errDesc =
        "Invalid value of Aggressive CPU Power Correction or Exception Action "
        "invalId for the given policy type";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidValue: Invalid value of "
        "Aggressive CPU Power Correction or Exception Action invalId for the "
        "given policy type";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidValue);
    }
};
struct NoPolicyIsCurrentlyLimiting final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.NoPolicyIsCurrentlyLimiting";
    static constexpr auto errDesc =
        "No policy is currently limiting for the specified Domain Id";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.NoPolicyIsCurrentlyLimiting: No "
        "policy is currently limiting for the specified Domain Id";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::NoPolicyIsCurrentlyLimiting);
    }
};
struct OtherErrorEncountered final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.OtherErrorEncountered";
    static constexpr auto errDesc = "Other error encountered";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.OtherErrorEncountered: Other "
        "error encountered";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::OtherErrorEncountered);
    }
};
struct ParameterOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.ParameterOutOfRange";
    static constexpr auto errDesc =
        "Returned when Minimum Power Draw Range exceeds the maximum one";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.ParameterOutOfRange: Returned "
        "when Minimum Power Draw Range exceeds the maximum one";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::ParameterOutOfRange);
    }
};
struct PlatformNotInS0NorS1State final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PlatformNotInS0NorS1State";
    static constexpr auto errDesc = "Platform not in S0 nor S1 state";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.PlatformNotInS0NorS1State: "
        "Platform not in S0 nor S1 state";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PlatformNotInS0NorS1State);
    }
};
struct PoliciesCannotBeCreated final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PoliciesCannotBeCreated";
    static constexpr auto errDesc =
        "Policies in given power domain cannot be created in the current "
        "configuration e.g., attempt to create predictive power limiting "
        "policy in DC power domain";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.PoliciesCannotBeCreated: "
        "Policies in given power domain cannot be created in the current "
        "configuration e.g., attempt to create predictive power limiting "
        "policy in DC power domain";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PoliciesCannotBeCreated);
    }
};
struct PolicyIdAlreadyExists final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PolicyIdAlreadyExists";
    static constexpr auto errDesc =
        "Policy could not be updated since Policy Id already exists and one of "
        "it parameters, which is changing, is not modifiable when policy is "
        "enabled";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.PolicyIdAlreadyExists: Policy "
        "could not be updated since Policy Id already exists and one of it "
        "parameters, which is changing, is not modifiable when policy is "
        "enabled";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PolicyIdAlreadyExists);
    }
};
struct TriggerValueOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.TriggerValueOutOfRange";
    static constexpr auto errDesc = "Policy Trigger value out of range";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.TriggerValueOutOfRange: "
        "Policy Trigger value out of range";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::TriggerValueOutOfRange);
    }
};
struct PowerDomainNotSupported final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PowerDomainNotSupported";
    static constexpr auto errDesc =
        "Power domain not supported for given Domain Id and Mode combination";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.PowerDomainNotSupported: Power "
        "domain not supported for given Domain Id and Mode combination";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PowerDomainNotSupported);
    }
};
struct PowerLimitNotSet final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PowerLimitNotSet";
    static constexpr auto errDesc = "Power Limit not set";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "PowerLimitNotSet: Power Limit not set";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::PowerLimitNotSet);
    }
};
struct PowerLimitOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PowerLimitOutOfRange";
    static constexpr auto errDesc = "Power Limit out of range";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.PowerLimitOutOfRange: Power "
        "Limit out of range";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::PowerLimitOutOfRange);
    }
};
struct PstateOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.PstateOutOfRange";
    static constexpr auto errDesc = "Pstate out of range";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "PstateOutOfRange: Pstate out of range";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::PstateOutOfRange);
    }
};
struct ReadingSourceUnavailable final : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.NodeManager.Error."
                                    "ReadingSourceUnavailable";
    static constexpr auto errDesc = "Reading source unavailable";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "ReadingSourceUnavailable: "
                                    "Reading source unavailable";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::ReadingSourceUnavailable);
    }
};
struct StatRepPeriodOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.NodeManager.Error."
                                    "StatRepPeriodOutOfRange";
    static constexpr auto errDesc = "Statistics Reporting Period out of range";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "StatRepPeriodOutOfRange: "
                                    "Statistics Reporting Period out of range";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::StatRepPeriodOutOfRange);
    }
};
struct TurboRatioLimitOutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.TurboRatioLimitOutOfRange";
    static constexpr auto errDesc =
        "Value of Turbo Ratio Limit is outsIde of valId min max range";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.TurboRatioLimitOutOfRange: "
        "Value of Turbo Ratio Limit is outsIde of valId min max range";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::TurboRatioLimitOutOfRange);
    }
};
struct UnsupportedPolicyTriggerType final : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.NodeManager.Error."
                                    "UnsupportedPolicyTriggerType";
    static constexpr auto errDesc =
        "Unknown or unsupported Policy Trigger Type";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error."
        "UnsupportedPolicyTriggerType: Unknown or unsupported Policy "
        "Trigger Type";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::UnsupportedPolicyTriggerType);
    }
};
struct UnknownPolicyType final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.UnknownPolicyType";
    static constexpr auto errDesc = "Unknown Policy Type";
    static constexpr auto errWhat = "xyz.openbmc_project.NodeManager.Error."
                                    "UnknownPolicyType: Unknown Policy Type";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::UnknownPolicyType);
    }
};
struct WrongValueOfCpuSockets final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.WrongValueOfCpuSockets";
    static constexpr auto errDesc = "Wrong value of CPU Socket Number";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.WrongValueOfCpuSockets: "
        "Wrong value of CPU Socket Number";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::WrongValueOfCpuSockets);
    }
};
struct InvalidPolicyStorage final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidPolicyStorage";
    static constexpr auto errDesc = "Invalid Policy Storage value";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidPolicyStorage: "
        "Invalid Policy Storage value";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InvalidPolicyStorage);
    }
};
struct InvalidLimitException final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidLimitException";
    static constexpr auto errDesc = "Invalid Limit Exception value";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidLimitException: "
        "Invalid Limit Exception value";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InvalidLimitException);
    }
};
struct InvalidPowerCorrectionType final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidPowerCorrectionType";
    static constexpr auto errDesc = "Invalid Power Correction Type value";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidPowerCorrectionType: "
        "Invalid Power Correction Type value";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::InvalidPowerCorrectionType);
    }
};

struct InvalidLogLevel final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.InvalidLogLevel";
    static constexpr auto errDesc = "Invalid Log Level value";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.InvalidLogLevel: "
        "Invalid Log Level value";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::InvalidLogLevel);
    }
};

struct OutOfRange final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.OutOfRange";
    static constexpr auto errDesc = "Out of range value";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.OutOfRange: "
        "Out of range value";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(ErrorCodes::OutOfRange);
    }
};

struct OperationNotPermitted final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.NodeManager.Error.OperationNotPermitted";
    static constexpr auto errDesc = "Operation not permitted";
    static constexpr auto errWhat =
        "xyz.openbmc_project.NodeManager.Error.OperationNotPermitted: "
        "Operation not permitted";
    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    int get_errno() const noexcept override
    {
        return std::underlying_type_t<ErrorCodes>(
            ErrorCodes::OperationNotPermitted);
    }
};

} // namespace errors
} // namespace nodemanager
