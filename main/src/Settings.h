#ifndef PERSE_ROVER_SETTINGS_H
#define PERSE_ROVER_SETTINGS_H

#include <nvs.h>
#include <cstring>

struct SettingsStruct {
	bool cameraHorizontalFlip = true;
	char wifiSSID[32] = "RoverNetwork";
	char wifiPassword[64] = "RoverRover";
};

class Settings {
public:
	Settings();

	SettingsStruct get();
	void set(SettingsStruct& settings);
	void store();

private:
	nvs_handle_t handle{};
	SettingsStruct settingsStruct;

	static constexpr const char* NVSNamespace = "Rover";
	static constexpr const char* BlobName = "Settings";

	void load();
};



#endif //PERSE_ROVER_SETTINGS_H
