#ifndef PERSE_ROVER_MICROROSSERVICE_H
#define PERSE_ROVER_MICROROSSERVICE_H

#include "Util/Threaded.h"
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

class MicroROSService : private Threaded {
public:
	MicroROSService();
	~MicroROSService() override;

	bool isInitialized() const;

private:
	void loop() override;
	bool onStart() override;
	void onStop() override;
	
	void cleanup();

	bool initialized = false;

	rcl_allocator_t allocator;
	rclc_support_t support;
	rcl_node_t node;
	rcl_publisher_t publisher;
	rclc_executor_t executor;
	rcl_timer_t timer;

	std_msgs__msg__Int32 msg;

	static void timerCallback(rcl_timer_t* timer, int64_t last_call_time);
};

#endif //PERSE_ROVER_MICROROSSERVICE_H
