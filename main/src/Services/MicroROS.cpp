#include "MicroROS.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <esp_log.h>
#include <cmath>

#include <uros_network_interfaces.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/float32.h>
#include <geometry_msgs/msg/twist.h>
#include <sensor_msgs/msg/battery_state.h>

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
#include <rmw_microros/rmw_microros.h>
#endif

#include "Util/Services.h"
#include "Devices/Battery.h"
#include "Devices/MotorDriveController.h"
#include "CommData.h"

static const char* TAG = "MicroROS";

// Error handling macros
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ \
    ESP_LOGE(TAG, "Failed status on line %d: %d. Aborting.", __LINE__, (int)temp_rc); \
    vTaskDelete(NULL); }}

#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ \
    ESP_LOGW(TAG, "Failed status on line %d: %d. Continuing.", __LINE__, (int)temp_rc); }}

// ROS objects
static rcl_publisher_t battery_publisher;
static rcl_subscription_t cmd_vel_subscriber;
static rcl_timer_t battery_timer;

// ROS messages
static sensor_msgs__msg__BatteryState battery_msg;
static geometry_msgs__msg__Twist cmd_vel_msg;

// Timer callback for publishing battery status
void battery_timer_callback(rcl_timer_t* timer, int64_t last_call_time) {
    (void) last_call_time;
    if (timer != NULL) {
        Battery* battery = (Battery*) Services.get(Service::Battery);
        if (battery != nullptr) {
            battery_msg.percentage = (float)battery->getPerc() / 100.0f;
            battery_msg.voltage = NAN; // Could be extended to get actual voltage
            battery_msg.current = NAN;
            battery_msg.capacity = NAN;
            battery_msg.design_capacity = NAN;
            battery_msg.charge = NAN;
            battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_DISCHARGING;
            battery_msg.power_supply_health = sensor_msgs__msg__BatteryState__POWER_SUPPLY_HEALTH_UNKNOWN;
            battery_msg.power_supply_technology = sensor_msgs__msg__BatteryState__POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
            battery_msg.present = true;

            RCSOFTCHECK(rcl_publish(&battery_publisher, &battery_msg, NULL));
            ESP_LOGI(TAG, "Published battery: %.1f%%", battery_msg.percentage * 100.0f);
        }
    }
}

// Subscription callback for cmd_vel messages
void cmd_vel_subscription_callback(const void* msgin) {
    const geometry_msgs__msg__Twist* msg = (const geometry_msgs__msg__Twist*)msgin;
    
    ESP_LOGI(TAG, "Received cmd_vel - linear.x: %.2f, angular.z: %.2f", 
             msg->linear.x, msg->angular.z);
    
    MotorDriveController* motorController = (MotorDriveController*) Services.get(Service::MotorDriveController);
    if (motorController != nullptr) {
        // Convert Twist message to motor drive state
        // linear.x is forward/backward velocity (-1.0 to 1.0)
        // angular.z is rotational velocity (-1.0 to 1.0)
        
        float linear = msg->linear.x;
        float angular = msg->angular.z;
        
        // Simple differential drive calculation
        // For now, we'll create a simple mapping to the existing drive direction format
        MotorDriveState state;
        
        // Determine direction based on linear and angular velocities
        if (fabs(linear) < 0.01f && fabs(angular) < 0.01f) {
            // Stop
            state.DriveDirection.dir = 0;
            state.DriveDirection.speed = 0.0f;
        } else if (fabs(angular) < 0.01f) {
            // Pure forward/backward
            if (linear > 0) {
                state.DriveDirection.dir = 1; // Forward
            } else {
                state.DriveDirection.dir = 2; // Backward
            }
            state.DriveDirection.speed = fabs(linear);
        } else if (fabs(linear) < 0.01f) {
            // Pure rotation
            if (angular > 0) {
                state.DriveDirection.dir = 3; // Left
            } else {
                state.DriveDirection.dir = 4; // Right
            }
            state.DriveDirection.speed = fabs(angular);
        } else {
            // Mixed motion - use linear direction with speed adjusted by both
            if (linear > 0) {
                state.DriveDirection.dir = 1; // Forward
            } else {
                state.DriveDirection.dir = 2; // Backward
            }
            state.DriveDirection.speed = sqrt(linear * linear + angular * angular);
            if (state.DriveDirection.speed > 1.0f) {
                state.DriveDirection.speed = 1.0f;
            }
        }
        
        motorController->setRemotely(state);
    }
}

void MicroROS::microRosTask(void* arg) {
    (void) arg;
    
    ESP_LOGI(TAG, "Starting micro-ROS task");
    
    // Wait for WiFi to be initialized
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;

    // Create init_options
    rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
    RCCHECK(rcl_init_options_init(&init_options, allocator));

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
    rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);
    
    // Use static Agent IP and port from config
    RCCHECK(rmw_uros_options_set_udp_address(CONFIG_MICRO_ROS_AGENT_IP, CONFIG_MICRO_ROS_AGENT_PORT, rmw_options));
#endif

    // Create support structure
    RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));

    // Create node
    rcl_node_t node = rcl_get_zero_initialized_node();
    RCCHECK(rclc_node_init_default(&node, "perse_rover", "", &support));
    
    ESP_LOGI(TAG, "Node created: perse_rover");

    // Create battery status publisher
    RCCHECK(rclc_publisher_init_default(
        &battery_publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState),
        "battery_status"));
    
    ESP_LOGI(TAG, "Battery publisher created");

    // Create cmd_vel subscriber
    RCCHECK(rclc_subscription_init_default(
        &cmd_vel_subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "cmd_vel"));
    
    ESP_LOGI(TAG, "cmd_vel subscriber created");

    // Create battery status timer (publish at 1 Hz)
    const unsigned int battery_timer_timeout = 1000;
    RCCHECK(rclc_timer_init_default2(
        &battery_timer,
        &support,
        RCL_MS_TO_NS(battery_timer_timeout),
        battery_timer_callback,
        true));
    
    ESP_LOGI(TAG, "Battery timer created");

    // Initialize battery message
    sensor_msgs__msg__BatteryState__init(&battery_msg);
    battery_msg.header.frame_id.data = (char*)"battery";
    battery_msg.header.frame_id.size = strlen(battery_msg.header.frame_id.data);
    battery_msg.header.frame_id.capacity = battery_msg.header.frame_id.size + 1;

    // Initialize cmd_vel message
    geometry_msgs__msg__Twist__init(&cmd_vel_msg);

    // Create executor
    rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
    RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
    
    // Add timer and subscription to executor
    RCCHECK(rclc_executor_add_timer(&executor, &battery_timer));
    RCCHECK(rclc_executor_add_subscription(&executor, &cmd_vel_subscriber, &cmd_vel_msg, 
                                           &cmd_vel_subscription_callback, ON_NEW_DATA));
    
    ESP_LOGI(TAG, "Executor initialized and spinning");

    // Spin executor
    while(1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        usleep(10000);
    }

    // Cleanup (will never be reached in this implementation)
    RCCHECK(rcl_subscription_fini(&cmd_vel_subscriber, &node));
    RCCHECK(rcl_publisher_fini(&battery_publisher, &node));
    RCCHECK(rcl_node_fini(&node));

    vTaskDelete(NULL);
}

MicroROS::MicroROS() {
}

MicroROS::~MicroROS() {
    stop();
}

void MicroROS::begin() {
    ESP_LOGI(TAG, "Initializing micro-ROS");
    
#if defined(CONFIG_MICRO_ROS_ESP_NETIF_WLAN) || defined(CONFIG_MICRO_ROS_ESP_NETIF_ENET)
    ESP_ERROR_CHECK(uros_network_interface_initialize());
#endif

    // Create micro-ROS task
    xTaskCreate(
        microRosTask,
        "microros_task",
        CONFIG_MICRO_ROS_APP_STACK,
        NULL,
        CONFIG_MICRO_ROS_APP_TASK_PRIO,
        &taskHandle);
    
    ESP_LOGI(TAG, "micro-ROS task created");
}

void MicroROS::stop() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}
