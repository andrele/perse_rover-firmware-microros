#include "MicroROSService.h"
#include <esp_log.h>
#include <uros_network_interfaces.h>
#include <rcl/error_handling.h>

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
#include <rmw_microros/rmw_microros.h>
#endif

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ ESP_LOGE(TAG, "Failed status on line %d: %d", __LINE__, (int)temp_rc); return false; }}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){ ESP_LOGW(TAG, "Failed status on line %d: %d", __LINE__, (int)temp_rc); }}

static const char* TAG = "MicroROS";
// Static instance pointer for timer callback. Safe because only one instance is created
// during single-threaded initialization in main.cpp before the state machine starts.
static MicroROSService* instance = nullptr;

MicroROSService::MicroROSService() : Threaded("MicroROS", 16000, 5, 1) {
	instance = this;
}

MicroROSService::~MicroROSService() {
	stop();
	cleanup();
	instance = nullptr;
}

bool MicroROSService::isInitialized() const {
	return initialized;
}

void MicroROSService::cleanup() {
	if(initialized){
		rclc_executor_fini(&executor);
		rcl_timer_fini(&timer);
		rcl_publisher_fini(&publisher, &node);
		rcl_node_fini(&node);
		rclc_support_fini(&support);
		initialized = false;
	}
}

bool MicroROSService::onStart() {
	// Note: Network interface (WiFi AP) is already initialized by WiFiAP service
	// so we don't call uros_network_interface_initialize() here

	allocator = rcl_get_default_allocator();

	rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&init_options, allocator));

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
	rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);
	RCCHECK(rmw_uros_options_set_udp_address(CONFIG_MICRO_ROS_AGENT_IP, CONFIG_MICRO_ROS_AGENT_PORT, rmw_options));
#endif

	// Create init_options
	RCCHECK(rclc_support_init_with_options(&support, 0, nullptr, &init_options, &allocator));

	// Create node
	RCCHECK(rclc_node_init_default(&node, "perse_rover_node", "", &support));

	// Create publisher
	RCCHECK(rclc_publisher_init_default(
		&publisher,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"perse_rover_status"));

	// Create timer
	const unsigned int timer_timeout = 1000;
	RCCHECK(rclc_timer_init_default2(
		&timer,
		&support,
		RCL_MS_TO_NS(timer_timeout),
		timerCallback,
		true));

	// Create executor
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));

	msg.data = 0;
	initialized = true;

	ESP_LOGI(TAG, "micro-ROS node initialized successfully");
	return true;
}

void MicroROSService::onStop() {
	cleanup();
}

void MicroROSService::loop() {
	if(!initialized){
		return;
	}

	rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
	vTaskDelay(pdMS_TO_TICKS(10));
}

void MicroROSService::timerCallback(rcl_timer_t* timer, int64_t last_call_time) {
	(void)last_call_time;
	
	if(timer != nullptr && instance != nullptr && instance->initialized){
		ESP_LOGI(TAG, "Publishing: %d", (int)instance->msg.data);
		RCSOFTCHECK(rcl_publish(&instance->publisher, &instance->msg, nullptr));
		instance->msg.data++;
	}
}
