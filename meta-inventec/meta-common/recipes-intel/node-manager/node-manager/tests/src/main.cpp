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
#include "multi_component_tests/node_manager_end_to_end_mct.hpp"
#include "multi_component_tests/power_budgeting_control_flow_mct.hpp"
#include "unit_tests/actions/action_binary_test.hpp"
#include "unit_tests/actions/action_cpu_utilization_test.hpp"
#include "unit_tests/actions/action_gpio_test.hpp"
#include "unit_tests/actions/action_test.hpp"
#include "unit_tests/budgeting/budgeting_test.hpp"
#include "unit_tests/budgeting/compound_domain_budgeting_test.hpp"
#include "unit_tests/budgeting/efficiency_helper_test.hpp"
#include "unit_tests/budgeting/simple_domain_budgeting_test.hpp"
#include "unit_tests/control/balancer_test.hpp"
#include "unit_tests/control/control_test.hpp"
#include "unit_tests/control/scalability/cpu_scalability_test.hpp"
#include "unit_tests/control/scalability/proportional_capabilites_scalability_test.hpp"
#include "unit_tests/control/scalability/proportional_cpu_scalability_test.hpp"
#include "unit_tests/devices_manager/devices_manager_test.hpp"
#include "unit_tests/devices_manager/hwmon_file_provider_test.hpp"
#include "unit_tests/domains/capabilities/component_capabilities_test.hpp"
#include "unit_tests/domains/capabilities/domain_capabilities_test.hpp"
#include "unit_tests/domains/domain_hw_protection_test.hpp"
#include "unit_tests/domains/domain_pcie_test.hpp"
#include "unit_tests/domains/domain_test.hpp"
#include "unit_tests/efficiency_control_test.hpp"
#include "unit_tests/knobs/hwmon_knob_test.hpp"
#include "unit_tests/knobs/hwpm_knob_test.hpp"
#include "unit_tests/knobs/pcie_dbus_knob_test.hpp"
#include "unit_tests/knobs/prochot_ratio_knob_test.hpp"
#include "unit_tests/knobs/turbo_ratio_knob_test.hpp"
#include "unit_tests/policies/limit_exception_handler_test.hpp"
#include "unit_tests/policies/limit_exception_monitor_test.hpp"
#include "unit_tests/policies/policy_factory_test.hpp"
#include "unit_tests/policies/policy_state_disabled_test.hpp"
#include "unit_tests/policies/policy_state_pending_test.hpp"
#include "unit_tests/policies/policy_state_ready_test.hpp"
#include "unit_tests/policies/policy_state_selected_test.hpp"
#include "unit_tests/policies/policy_state_suspended_test.hpp"
#include "unit_tests/policies/policy_state_triggered_test.hpp"
#include "unit_tests/policies/policy_storage_management_test.hpp"
#include "unit_tests/policies/policy_test.hpp"
#include "unit_tests/policies/power_policy_test.hpp"
#include "unit_tests/readings/reading_ac_platform_limit_test.hpp"
#include "unit_tests/readings/reading_avg_test.hpp"
#include "unit_tests/readings/reading_cpu_utilization_test.hpp"
#include "unit_tests/readings/reading_delta_test.hpp"
#include "unit_tests/readings/reading_historical_max_test.hpp"
#include "unit_tests/readings/reading_max_test.hpp"
#include "unit_tests/readings/reading_min_test.hpp"
#include "unit_tests/regulator_p_test.hpp"
#include "unit_tests/sensors/cpu_efficiency_sensor_test.hpp"
#include "unit_tests/sensors/cpu_frequency_sensor_test.hpp"
#include "unit_tests/sensors/cpu_utilization_sensor_test.hpp"
#include "unit_tests/sensors/gpio_sensor_test.hpp"
#include "unit_tests/sensors/gpu_power_state_dbus_sensor_test.hpp"
#include "unit_tests/sensors/hwmon_sensor_test.hpp"
#include "unit_tests/sensors/peci_sensor_test.hpp"
#include "unit_tests/sensors/power_state_dbus_sensor_test.hpp"
#include "unit_tests/sensors/sensor_reading_test.hpp"
#include "unit_tests/sensors/sensor_readings_manager_test.hpp"
#include "unit_tests/sensors/sensor_test.hpp"
#include "unit_tests/statistics/energy_statistic_test.hpp"
#include "unit_tests/statistics/moving_average_test.hpp"
#include "unit_tests/statistics/normal_average_test.hpp"
#include "unit_tests/statistics/statistic_test.hpp"
#include "unit_tests/statistics/throttling_statistic_test.hpp"
#include "unit_tests/status_monitor_test.hpp"
#include "unit_tests/triggers/trigger_test.hpp"
#include "unit_tests/utility/async_executor_test.hpp"
#include "utils/dbus_environment.hpp"

#include "gtest/gtest.h"

int main(int argc, char** argv)
{
    auto env = new DbusEnvironment;

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(env);
    int ret = RUN_ALL_TESTS();
    env->teardown();
    return ret;
}
