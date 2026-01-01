# Micro-ROS Integration

This firmware integrates micro-ROS to enable ROS 2 communication with the Perseverance Space Rover.

## Features

- **ROS 2 Node**: `perse_rover` node running on ESP32-S3
- **Battery Status Publisher**: Publishes battery state at 1 Hz on `/battery_status` topic
- **Motor Control Subscriber**: Listens for velocity commands on `/cmd_vel` topic
- **Wi-Fi Transport**: Uses UDP transport over existing WiFi AP

## Prerequisites

1. ESP-IDF v5.1.1 (or compatible version)
2. Python dependencies:
   ```bash
   pip3 install catkin_pkg lark-parser colcon-common-extensions
   ```

3. micro-ROS Agent running on your host machine

## Building

1. Configure the project:
   ```bash
   idf.py menuconfig
   ```
   Navigate to `micro-ROS Settings` and configure:
   - Agent IP address (default: 192.168.4.2)
   - Agent port (default: 8888)
   - WiFi credentials

2. Build the firmware:
   ```bash
   idf.py build
   ```

3. Flash to device:
   ```bash
   idf.py -p <PORT> flash
   ```

## Running the micro-ROS Agent

On your host machine, run the micro-ROS agent with UDP transport:

```bash
docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888 -v6
```

Or use the latest version:

```bash
docker run -it --rm --net=host microros/micro-ros-agent:latest udp4 --port 8888 -v6
```

## ROS 2 Topics

### Published Topics

- **`/battery_status`** (`sensor_msgs/BatteryState`)
  - Battery percentage (0.0 - 1.0)
  - Published at 1 Hz
  - Power supply status

### Subscribed Topics

- **`/cmd_vel`** (`geometry_msgs/Twist`)
  - Linear velocity (x-axis): forward/backward motion
  - Angular velocity (z-axis): rotation
  - Range: -1.0 to 1.0 for both axes

## Testing with ROS 2

After starting the agent and flashing the firmware:

1. List available topics:
   ```bash
   ros2 topic list
   ```

2. Monitor battery status:
   ```bash
   ros2 topic echo /battery_status
   ```

3. Send velocity commands:
   ```bash
   # Move forward
   ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.5}, angular: {z: 0.0}}"
   
   # Rotate left
   ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.0}, angular: {z: 0.5}}"
   
   # Stop
   ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.0}, angular: {z: 0.0}}"
   ```

## Motor Control Mapping

The rover uses an 8-direction control scheme internally. The cmd_vel subscriber converts Twist messages:

- **Direction 0**: Stop (both velocities near 0)
- **Direction 1**: Forward (positive linear.x)
- **Direction 2**: Backward (negative linear.x)
- **Direction 3**: Left turn (positive angular.z)
- **Direction 4**: Right turn (negative angular.z)

Speed is calculated from the velocity magnitude, clamped to [0.0, 1.0].

## Architecture

### File Structure

```
main/
├── main.cpp                      # Modified to initialize MicroROS service
├── src/
    ├── Services/
        ├── MicroROS.h            # MicroROS service header
        └── MicroROS.cpp          # MicroROS implementation
    └── Util/
        └── Services.h            # Modified to include MicroROS service
```

### Integration Points

1. **Services Registry**: MicroROS registered as `Service::MicroROS`
2. **Battery Interface**: Uses existing `Devices/Battery.h`
3. **Motor Control**: Uses existing `Devices/MotorDriveController.h`
4. **WiFi**: Leverages existing `Periph/WiFiAP.h` for network connectivity

### FreeRTOS Task

The micro-ROS executor runs in a dedicated FreeRTOS task with:
- Stack size: `CONFIG_MICRO_ROS_APP_STACK` (configurable)
- Priority: `CONFIG_MICRO_ROS_APP_TASK_PRIO` (configurable)
- Core affinity: Pinned to APP_CPU to avoid WiFi interference

## Configuration

### colcon.meta

The `colcon.meta` file configures micro-ROS middleware limits:
- Max nodes: 1
- Max publishers: 3
- Max subscriptions: 2
- Max history: 4

Adjust these values if you need more publishers or subscribers.

### Kconfig Options

Available through `idf.py menuconfig` under `micro-ROS Settings`:
- Agent IP and port
- Transport type (UDP/Serial/Custom)
- Stack size and task priority
- Network interface selection

## Troubleshooting

### Agent Connection Issues

- Verify the rover's IP address matches the agent configuration
- Ensure the agent is running before the rover boots
- Check firewall settings on the host machine
- Confirm WiFi credentials are correct

### Build Issues

- Ensure all Python dependencies are installed
- Clean and rebuild: `idf.py clean-microros && idf.py build`
- Check ESP-IDF version compatibility

### Runtime Issues

- Monitor serial output: `idf.py monitor`
- Check for micro-ROS task creation in logs
- Verify WiFi connection is established before micro-ROS starts

## Extending the Implementation

### Adding More Publishers

1. Update `colcon.meta` to increase `DRMW_UXRCE_MAX_PUBLISHERS`
2. Add publisher declaration in `MicroROS.cpp`
3. Create timer callback or event-driven publishing
4. Update executor with additional publishers

### Adding More Subscribers

1. Update `colcon.meta` to increase `DRMW_UXRCE_MAX_SUBSCRIPTIONS`
2. Add subscriber declaration and callback in `MicroROS.cpp`
3. Update executor initialization
4. Integrate with existing device controllers

### IMU/Sensor Data Publishing

To add IMU or other sensor data:
1. Include appropriate ROS message types (e.g., `sensor_msgs/Imu`)
2. Create publisher and timer callback
3. Query sensor data from existing device services
4. Publish at appropriate rate

## License

Same as the main firmware repository.
