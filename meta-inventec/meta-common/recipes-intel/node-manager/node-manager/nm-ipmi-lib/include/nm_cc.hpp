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
#include "nm_dbus_errors.hpp"

#include <ipmid/api.hpp>

namespace nmipmi
{

// IPMI NM completion codes specified by the Intel OpenBMC Node Manager External
// Interface Specification
//
constexpr ipmi::Cc ccInvalidComponentIdentifier = 0x80; // Invalid Component
                                                        // Identifier
constexpr ipmi::Cc ccInvalidPolicyId = 0x80;            // Invalid Policy Id
constexpr ipmi::Cc ccPowerLimitNotSet = 0x80;           // Power Limit not set
constexpr ipmi::Cc ccCommandNotSupported =
    0x81; // Command not supported in the current configuration. Returned when
          // PSU is configured as the power reading source
constexpr ipmi::Cc ccInvalidDomainId = 0x81; // Invalid Domain Id
constexpr ipmi::Cc ccUnsupportedPolicyTriggerType =
    0x82; // Unknown or unsupported Policy Trigger Type
constexpr ipmi::Cc ccUnknownPolicyType = 0x83;     // Unknown Policy Type
constexpr ipmi::Cc ccPowerBudgetOutOfRange = 0x84; // Power Budget out of range
constexpr ipmi::Cc ccPowerLimitOutOfRange = 0x84;  // Power Limit out of range
constexpr ipmi::Cc ccCorrectionTimeOutOfRange =
    0x85; // Correction Time out of range
constexpr ipmi::Cc ccTriggerValueOutOfRange =
    0x86;                                // Policy Trigger value out of range
constexpr ipmi::Cc ccInvalidMode = 0x88; // Invalid Mode
constexpr ipmi::Cc ccStatRepPeriodOutOfRange =
    0x89; // Statistics Reporting Period out of range
constexpr ipmi::Cc ccPstateOutOfRange = 0x8A; // Pstate out of range
constexpr ipmi::Cc ccInvalidValue =
    0x8B; // Invalid value of Aggressive CPU Power Correction or Exception
          // Action Invalid for the given policy type
constexpr ipmi::Cc ccIncorrectKCoefficientValue =
    0xA1; // Incorrect K Coefficient value
constexpr ipmi::Cc ccNoPolicyIsCurrentlyLimiting =
    0xA1; // No policy is currently limiting for the specified Domain Id
constexpr ipmi::Cc ccWrongValueOfCPUSocketNumber =
    0xA1; // Wrong value of CPU Socket Number
constexpr ipmi::Cc ccCmdResponseTimeout = 0xA2; // Command response timeout
constexpr ipmi::Cc ccBadReadFCSIn = 0xA4;       // Bad read FCS in the response
constexpr ipmi::Cc ccBadWriteFCSFieldIn =
    0xA5; // Bad write FCS field in the response
constexpr ipmi::Cc ccReadingSourceNotAvailable =
    0xA6;                                  // Reading source not available
constexpr ipmi::Cc ccCPUNotPresent = 0xAC; // CPU not present
constexpr ipmi::Cc ccDCMIModeIsNotPresent =
    0xC1; // Returned when DCMI mode is not present
constexpr ipmi::Cc ccIncorrectActiveCoresConfiguration =
    0xC9; // Unsupported number of active cores
constexpr ipmi::Cc ccParameterOutOfRange =
    0xC9; // Returned when Minimum Power Draw Range exceeds the maximum one
constexpr ipmi::Cc ccPowerDomainNotSupported =
    0xCC; // Power domain not supported for given Domain Id and Mode combination
constexpr ipmi::Cc ccTurboRatioLimitOutOfRange =
    0xCC; // Value of Turbo Ratio Limit is outsIde of valId min max range
constexpr ipmi::Cc ccInsufficientPrivilegeLevel =
    0xD4; // Insufficient privilege level due wrong LUN
constexpr ipmi::Cc ccHWPAreEnabledInBIOS =
    0xD5; // Hardware Controlled Performance States (HWP) are enabled in BIOS.
          // Legacy ACPI Pstate setting is not supported
constexpr ipmi::Cc ccPlatformNotInS0NorS1State =
    0xD5; // Platform not in S0 nor S1 state
constexpr ipmi::Cc ccPolicyIdAlreadyExists =
    0xD5; // Policy could not be updated since Policy Id already exists and one
          // of it parameters, which is changing, is not modifiable when policy
          // is enabled
constexpr ipmi::Cc ccPoliciesCannotBeCreated =
    0xD6; // Policies in given power domain cannot be created in the current
          // configuration e.g., attempt to create predictive power limiting
          // policy in DC power domain

} // namespace nmipmi