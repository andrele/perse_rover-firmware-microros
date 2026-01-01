# Perseverance Space Rover Firmware

# WiFi Configuration

This firmware has been refactored to use WiFi Station (STA) mode instead of Access Point (AP) mode. This change enables the rover to connect to an existing WiFi network, which is required for microROS integration.

## Default WiFi Credentials

The default WiFi credentials are stored in the Settings structure and can be configured:

- **SSID**: `RoverNetwork`
- **Password**: `RoverRover`

These credentials are stored in NVS (Non-Volatile Storage) and persist across reboots. You can modify the default values in `main/src/Settings.h` or implement a configuration interface to change them at runtime.

**Security Note**: WiFi credentials are stored in plain text in NVS as per standard ESP-IDF practices. For production deployments requiring enhanced security, consider implementing NVS encryption as documented in the [ESP-IDF NVS Encryption guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html#nvs-encryption).

## WiFi Connection Behavior

- The rover automatically connects to the configured WiFi network on startup
- If the connection is lost, the rover will automatically attempt to reconnect
- WiFi events (connect, disconnect, got IP) are posted to the event system for monitoring
- The PairService now waits for WiFi connection before accepting TCP connections

# Building

To build the Rover firmware, you'll need the ESP-IDF. You can find the getting
started guide [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).

The production firmware is built using IDF
version [5.1.1](https://github.com/espressif/esp-idf/releases/tag/v5.1.1).

In the root directory of the project:

**To build the firmware** run ```idf.py  build```

**To upload the firmware to the device** run ```idf.py -p <PORT> flash```.
Replace `<PORT>` with the port the Rover is attached to, for ex. ```COM6``` or
```/dev/ttyACM0```.


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
