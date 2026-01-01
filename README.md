# Perseverance Space Rover Firmware

# Building

To build the Rover firmware, you'll need the ESP-IDF. You can find the getting
started guide [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).

The production firmware is built using IDF
version [5.1.1](https://github.com/espressif/esp-idf/releases/tag/v5.1.1).

## Prerequisites for micro-ROS

This firmware includes micro-ROS support. Before building, install the required Python dependencies:

```bash
. $IDF_PATH/export.sh
pip3 install catkin_pkg lark-parser colcon-common-extensions
```

## Building and Flashing

In the root directory of the project:

**To configure micro-ROS settings** run ```idf.py menuconfig```
- Navigate to "micro-ROS Settings" to configure:
  - micro-ROS Agent IP address (default: 192.168.4.2)
  - micro-ROS Agent port (default: 8888)
  - Network interface (WiFi/Ethernet)

**To build the firmware** run ```idf.py  build```

**To upload the firmware to the device** run ```idf.py -p <PORT> flash```.
Replace `<PORT>` with the port the Rover is attached to, for ex. ```COM6``` or
```/dev/ttyACM0```.

## Running micro-ROS Agent

To connect to the rover's micro-ROS node, run the micro-ROS agent on your host computer:

```bash
# Using Docker
docker run -it --rm --net=host microros/micro-ros-agent:humble udp4 --port 8888 -v6
```

The rover will publish status messages to the `perse_rover_status` topic.


# Restoring the stock firmware

To restore the stock firmware, you can download the prebuilt binary on
the [releases page](https://github.com/CircuitMess/Perse_Rover-Firmware/releases) of this repository
and flash it manually using esptool:

```shell
esptool -c esp32s3 -b 921600 -p <PORT> write_flash 0 Rover-Firmware.bin
```

Alternatively, you can also do so using [CircuitBlocks](https://code.circuitmess.com/) by
logging in, clicking the "Restore Firmware" button in the top-right corner, and following the
on-screen instructions.
