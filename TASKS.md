# Micro-ROS Porting Roadmap

- [ ] **Dependencies**: Add `micro_ros_espidf_component` as a submodule in `components/`.
- [ ] **Build System**: Update `main/CMakeLists.txt` and `CMakeLists.txt` to link against the Micro-ROS component.
- [ ] **Configuration**: Create `colcon.meta` if necessary to configure the specific middleware/transport for ESP32-S3.
- [ ] **Entry Point**: Refactor `main/main.cpp` to initialize the Micro-ROS allocator and support structures.
- [ ] **Transport**: Configure the Wi-Fi transport (using `Periph/WiFiAP.h` or standard ESP-IDF Wi-Fi) for Micro-ROS.
- [ ] **ROS Node**: Create a standard ROS 2 node.
- [ ] **Publishers**:
    - [ ] Battery status (from `Devices/Battery.h`)
    - [ ] Sensor data (IMU, etc.)
- [ ] **Subscribers**:
    - [ ] `cmd_vel` geometry_msgs/Twist to drive motors (`Devices/MotorDriveController.h`).
- [ ] **Executors**: Implement a dedicated FreeRTOS task to spin the Micro-ROS executor.
