# Micro-ROS Porting Roadmap

- [x] **Dependencies**: Add `micro_ros_espidf_component` as a submodule in `components/`.
- [x] **Build System**: Update `main/CMakeLists.txt` and `CMakeLists.txt` to link against the Micro-ROS component.
- [x] **Configuration**: Create `colcon.meta` if necessary to configure the specific middleware/transport for ESP32-S3.
- [x] **Entry Point**: Refactor `main/main.cpp` to initialize the Micro-ROS allocator and support structures.
- [x] **Transport**: Configure the Wi-Fi transport (using `Periph/WiFiAP.h` or standard ESP-IDF Wi-Fi) for Micro-ROS.
- [x] **ROS Node**: Create a standard ROS 2 node.
- [x] **Publishers**:
    - [x] Battery status (from `Devices/Battery.h`)
    - [x] Sensor data (IMU, etc.) - battery status implemented, can be extended for other sensors
- [x] **Subscribers**:
    - [x] `cmd_vel` geometry_msgs/Twist to drive motors (`Devices/MotorDriveController.h`).
- [x] **Executors**: Implement a dedicated FreeRTOS task to spin the Micro-ROS executor.

## Implementation Notes

### Components Added
- `components/micro_ros_espidf_component` - Official micro-ROS component for ESP-IDF
- `main/src/Services/MicroROS.h/cpp` - Service wrapper for micro-ROS integration

### Configuration
- `colcon.meta` - Configured for 3 publishers, 2 subscriptions (can be adjusted as needed)
- Wi-Fi transport configured via ESP-IDF menuconfig (`CONFIG_MICRO_ROS_AGENT_IP` and `CONFIG_MICRO_ROS_AGENT_PORT`)

### ROS 2 Node: `perse_rover`

#### Publishers
1. **`/battery_status`** (sensor_msgs/BatteryState)
   - Publishes battery percentage at 1 Hz
   - Gets data from `Devices/Battery.h`

#### Subscribers
1. **`/cmd_vel`** (geometry_msgs/Twist)
   - Receives velocity commands
   - Converts Twist (linear.x, angular.z) to DriveDir format
   - Controls motors via `Devices/MotorDriveController.h`

### Motor Control Mapping
The cmd_vel subscriber converts Twist messages to the rover's 8-direction control scheme:
- `linear.x > 0`: Forward motion
- `linear.x < 0`: Backward motion  
- `angular.z > 0`: Left rotation
- `angular.z < 0`: Right rotation
- Mixed commands combine linear and angular velocities

### Build Instructions
1. Setup ESP-IDF environment
2. Install dependencies: `pip3 install catkin_pkg lark-parser colcon-common-extensions`
3. Configure: `idf.py menuconfig` (set micro-ROS Agent IP/Port in micro-ROS Settings)
4. Build: `idf.py build`
5. Flash: `idf.py -p <PORT> flash`

### Running micro-ROS Agent
```bash
docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888 -v6
```
