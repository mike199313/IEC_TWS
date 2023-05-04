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
#include <ipmid/api-types.hpp>

namespace nmipmi
{
namespace intel
{
static constexpr ipmi::NetFn netFnGeneral = ipmi::netFnOem;
static constexpr ipmi::NetFn netFnDcmi = ipmi::netFnGroup;

namespace oem
{
using Number = std::uint32_t;

static constexpr Number intelOemNumber = 343;
} // namespace oem

namespace general
{
static constexpr ipmi::Cmd cmdEnableNmPolicyCtrl = 0xC0;
static constexpr ipmi::Cmd cmdSetNmPolicy = 0xC1;
static constexpr ipmi::Cmd cmdGetNmPolicy = 0xC2;
static constexpr ipmi::Cmd cmdSetNmPolicyAlertThresholds = 0xC3;
static constexpr ipmi::Cmd cmdGetNmPolicyAlertThresholds = 0xC4;
static constexpr ipmi::Cmd cmdSetNmPolicySuspendPeriods = 0xC5;
static constexpr ipmi::Cmd cmdGetNmPolicySuspendPeriods = 0xC6;
static constexpr ipmi::Cmd cmdResetNmStat = 0xC7;
static constexpr ipmi::Cmd cmdGetNmStat = 0xC8;
static constexpr ipmi::Cmd cmdGetNmCapab = 0xC9;
static constexpr ipmi::Cmd cmdGetNmVersion = 0xCA;
static constexpr ipmi::Cmd cmdSetNmPowerDrawRange = 0xCB;
static constexpr ipmi::Cmd cmdSetTurboSyncRatio = 0xCC;
static constexpr ipmi::Cmd cmdGetTurboSyncRatio = 0xCD;
static constexpr ipmi::Cmd cmdSetTotalPowerBudget = 0xD0;
static constexpr ipmi::Cmd cmdGetTotalPowerBudget = 0xD1;
static constexpr ipmi::Cmd cmdSetMaxCpuPstateThreads = 0xD2;
static constexpr ipmi::Cmd cmdGetMaxCpuPstateThreads = 0xD3;
static constexpr ipmi::Cmd cmdGetNumberOfCpuPstateThreads = 0xD4;
static constexpr ipmi::Cmd cmdSetHwProtectionCoeff = 0xF0;
static constexpr ipmi::Cmd cmdGetHwProtectionCoeff = 0xF1;
static constexpr ipmi::Cmd cmdGetLimitingPolicyId = 0xF2;
} // namespace general

namespace DCMI
{
static constexpr ipmi::Cmd cmdGetDcmiCapabInfo = 0x01;
static constexpr ipmi::Cmd cmdGetPowerReading = 0x02;
static constexpr ipmi::Cmd cmdGetPowerLimit = 0x03;
static constexpr ipmi::Cmd cmdSetPowerLimit = 0x04;
static constexpr ipmi::Cmd cmdActivatePowerLimit = 0x05;
} // namespace DCMI

} // namespace intel

//-------------------------------------------------------------------

// Below definitions maps dbus nm error codes to IPMI ComplitionCodes

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetDcmiCapabInfoErr = {
        {ErrorCodes::DcmiModeIsNotPresent, ccDCMIModeIsNotPresent}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetPowerReadingErr = {
        {ErrorCodes::DcmiModeIsNotPresent, ccDCMIModeIsNotPresent}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetPowerLimitErr = {
        {ErrorCodes::PowerLimitNotSet, ccPowerLimitNotSet},
        {ErrorCodes::DcmiModeIsNotPresent, ccDCMIModeIsNotPresent}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kSetPowerLimitErr = {
        {ErrorCodes::PowerLimitOutOfRange, ccPowerLimitOutOfRange},
        {ErrorCodes::CorrectionTimeOutOfRange, ccCorrectionTimeOutOfRange},
        {ErrorCodes::StatRepPeriodOutOfRange, ccStatRepPeriodOutOfRange},
        {ErrorCodes::DcmiModeIsNotPresent, ccDCMIModeIsNotPresent}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kActivatePowerLimitErr = {
        {ErrorCodes::DcmiModeIsNotPresent, ccDCMIModeIsNotPresent}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kEnableNmPolicyCtrlErr = {
        {ErrorCodes::InvalidPolicyId, ccInvalidPolicyId},
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
        {ErrorCodes::InsufficientPrivilegeLevel, ccInsufficientPrivilegeLevel}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc> kSetNmPolicyErr =
    {{ErrorCodes::InvalidPolicyId, ccInvalidPolicyId},
     {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
     {ErrorCodes::UnsupportedPolicyTriggerType, ccUnsupportedPolicyTriggerType},
     {ErrorCodes::PowerLimitOutOfRange, ccPowerLimitOutOfRange},
     {ErrorCodes::CorrectionTimeOutOfRange, ccCorrectionTimeOutOfRange},
     {ErrorCodes::TriggerValueOutOfRange, ccTriggerValueOutOfRange},
     {ErrorCodes::StatRepPeriodOutOfRange, ccStatRepPeriodOutOfRange},
     {ErrorCodes::InvalidValue, ccInvalidValue},
     {ErrorCodes::InsufficientPrivilegeLevel, ccInsufficientPrivilegeLevel},
     {ErrorCodes::PolicyIdAlreadyExists, ccPolicyIdAlreadyExists},
     {ErrorCodes::PoliciesCannotBeCreated, ccPoliciesCannotBeCreated},
     {ErrorCodes::InvalidArgument, ipmi::ccInvalidFieldRequest},
     {ErrorCodes::InvalidPolicyStorage, ipmi::ccInvalidFieldRequest},
     {ErrorCodes::InvalidLimitException, ccInvalidValue},
     {ErrorCodes::InvalidPowerCorrectionType, ccInvalidValue},
     {ErrorCodes::InvalidComponentId, ccInvalidComponentIdentifier},
     {ErrorCodes::ReadingSourceUnavailable, ccReadingSourceNotAvailable},
     {ErrorCodes::OperationNotPermitted, ccInvalidDomainId}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc> kGetNmPolicyErr =
    {{ErrorCodes::InvalidPolicyId, ccInvalidPolicyId},
     {ErrorCodes::InvalidDomainId, ccInvalidDomainId}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc> kResetNmStatErr =
    {{ErrorCodes::InvalidPolicyId, ccInvalidPolicyId},
     {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
     {ErrorCodes::InvalidMode, ccInvalidMode},
     {ErrorCodes::InsufficientPrivilegeLevel, ccInsufficientPrivilegeLevel}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc> kGetNmStatErr = {
    {ErrorCodes::InvalidComponentId, ccInvalidComponentIdentifier},
    {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
    {ErrorCodes::InvalidMode, ccInvalidMode},
    {ErrorCodes::PowerDomainNotSupported, ccPowerDomainNotSupported}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc> kGetNmCapabErr = {
    {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
    {ErrorCodes::UnsupportedPolicyTriggerType, ccUnsupportedPolicyTriggerType},
    {ErrorCodes::UnknownPolicyType, ccUnknownPolicyType}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kSetNmPowerDrawRangeErr = {
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
        {ErrorCodes::ParameterOutOfRange, ccParameterOutOfRange},
        {ErrorCodes::CmdNotSupported, ccCommandNotSupported}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kSetTurboSyncRatioErr = {
        {ErrorCodes::WrongValueOfCpuSockets, ccWrongValueOfCPUSocketNumber},
        {ErrorCodes::CmdResponseTimeout, ccCmdResponseTimeout},
        {ErrorCodes::BadReadFcsIn, ccBadReadFCSIn},
        {ErrorCodes::BadWriteFcsField, ccBadWriteFCSFieldIn},
        {ErrorCodes::CpuNotPresent, ccCPUNotPresent},
        {ErrorCodes::IncorrectActiveCoresConfiguration,
         ccIncorrectActiveCoresConfiguration},
        {ErrorCodes::TurboRatioLimitOutOfRange, ccTurboRatioLimitOutOfRange},
        {ErrorCodes::PlatformNotInS0NorS1State, ccPlatformNotInS0NorS1State},
        {ErrorCodes::OtherErrorEncountered, ipmi::ccUnspecifiedError}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetTurboSyncRatioErr = {
        {ErrorCodes::WrongValueOfCpuSockets, ccWrongValueOfCPUSocketNumber},
        {ErrorCodes::CmdResponseTimeout, ccCmdResponseTimeout},
        {ErrorCodes::BadReadFcsIn, ccBadReadFCSIn},
        {ErrorCodes::BadWriteFcsField, ccBadWriteFCSFieldIn},
        {ErrorCodes::CpuNotPresent, ccCPUNotPresent},
        {ErrorCodes::IncorrectActiveCoresConfiguration,
         ccIncorrectActiveCoresConfiguration},
        {ErrorCodes::PlatformNotInS0NorS1State, ccPlatformNotInS0NorS1State},
        {ErrorCodes::OtherErrorEncountered, ipmi::ccUnspecifiedError}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kSetTotalPowerBudgetErr = {
        {ErrorCodes::InvalidComponentId, ccInvalidComponentIdentifier},
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
        {ErrorCodes::PowerLimitOutOfRange, ccPowerBudgetOutOfRange}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetTotalPowerBudgetErr = {
        {ErrorCodes::InvalidComponentId, ccInvalidComponentIdentifier},
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kSetMaxCpuPstateThreadsErr = {
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
        {ErrorCodes::PstateOutOfRange, ccPstateOutOfRange},
        {ErrorCodes::HwpAreEnabledInBios, ccHWPAreEnabledInBIOS}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetMaxCpuPstateThreadsErr = {
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId},
        {ErrorCodes::HwpAreEnabledInBios, ccHWPAreEnabledInBIOS}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetNumberOfCpuPstateThreadsErr = {
        {ErrorCodes::InvalidDomainId, ccInvalidDomainId}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kSetHwProtectionCoeffErr = {
        {ErrorCodes::CmdNotSupported, ccCommandNotSupported},
        {ErrorCodes::IncorrectKCoefficientValue, ccIncorrectKCoefficientValue}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetHwProtectionCoeffErr = {
        {ErrorCodes::CmdNotSupported, ccCommandNotSupported}};

static const boost::container::flat_map<ErrorCodes, ipmi::Cc>
    kGetLimitingPolicyIdErr = {
        {ErrorCodes::CmdNotSupported, ccCommandNotSupported},
        {ErrorCodes::NoPolicyIsCurrentlyLimiting,
         ccNoPolicyIsCurrentlyLimiting}};

//------------------------------------------------------------------------------

// @brief This is a map of ipmi commands to corresponding error mappings.
static const boost::container::flat_map<
    ipmi::Cmd, boost::container::flat_map<ErrorCodes, ipmi::Cc>>
    kCmdCcErrnoMap = {
        {intel::DCMI::cmdGetDcmiCapabInfo, kGetDcmiCapabInfoErr},
        {intel::DCMI::cmdGetPowerReading, kGetPowerReadingErr},
        {intel::DCMI::cmdGetPowerLimit, kGetPowerLimitErr},
        {intel::DCMI::cmdSetPowerLimit, kSetPowerLimitErr},
        {intel::DCMI::cmdActivatePowerLimit, kActivatePowerLimitErr},
        {intel::general::cmdEnableNmPolicyCtrl, kEnableNmPolicyCtrlErr},
        {intel::general::cmdSetNmPolicy, kSetNmPolicyErr},
        {intel::general::cmdGetNmPolicy, kGetNmPolicyErr},
        {intel::general::cmdResetNmStat, kResetNmStatErr},
        {intel::general::cmdGetNmStat, kGetNmStatErr},
        {intel::general::cmdGetNmCapab, kGetNmCapabErr},
        {intel::general::cmdSetNmPowerDrawRange, kSetNmPowerDrawRangeErr},
        {intel::general::cmdSetTurboSyncRatio, kSetTurboSyncRatioErr},
        {intel::general::cmdGetTurboSyncRatio, kGetTurboSyncRatioErr},
        {intel::general::cmdSetTotalPowerBudget, kSetTotalPowerBudgetErr},
        {intel::general::cmdGetTotalPowerBudget, kGetTotalPowerBudgetErr},
        {intel::general::cmdSetMaxCpuPstateThreads, kSetMaxCpuPstateThreadsErr},
        {intel::general::cmdGetMaxCpuPstateThreads, kGetMaxCpuPstateThreadsErr},
        {intel::general::cmdGetNumberOfCpuPstateThreads,
         kGetNumberOfCpuPstateThreadsErr},
        {intel::general::cmdSetHwProtectionCoeff, kSetHwProtectionCoeffErr},
        {intel::general::cmdGetHwProtectionCoeff, kGetHwProtectionCoeffErr},
        {intel::general::cmdGetLimitingPolicyId, kGetLimitingPolicyIdErr},
};

} // namespace nmipmi