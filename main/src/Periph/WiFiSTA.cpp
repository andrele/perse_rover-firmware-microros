#include "WiFiSTA.h"
#include "Util/Events.h"
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <string>
#include <cstring>
#include <esp_log.h>

static const char* TAG = "WiFi_STA";

WiFiSTA::WiFiSTA(const char* ssid, const char* password) : connected(false), netif(nullptr) {
	// Create event loop if it doesn't exist, otherwise use existing one
	esp_err_t ret = esp_event_loop_create_default();
	if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "Failed to create default event loop: %d", ret);
		ESP_ERROR_CHECK(ret);
	}
	
	// Register WiFi event handler
	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, [](void* arg, esp_event_base_t base, int32_t id, void* data){
		auto wifi = static_cast<WiFiSTA*>(arg);
		wifi->event(base, id, data);
	}, this, &wifiEvtHandler);
	
	// Register IP event handler
	esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, [](void* arg, esp_event_base_t base, int32_t id, void* data){
		auto wifi = static_cast<WiFiSTA*>(arg);
		wifi->event(base, id, data);
	}, this, &ipEvtHandler);

	ESP_ERROR_CHECK(esp_netif_init());
	createNetif();

	const wifi_init_config_t cfg_wifi = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg_wifi));

	wifi_config_t cfg_sta = {};
	strncpy((char*) cfg_sta.sta.ssid, ssid, sizeof(cfg_sta.sta.ssid) - 1);
	cfg_sta.sta.ssid[sizeof(cfg_sta.sta.ssid) - 1] = '\0';
	strncpy((char*) cfg_sta.sta.password, password, sizeof(cfg_sta.sta.password) - 1);
	cfg_sta.sta.password[sizeof(cfg_sta.sta.password) - 1] = '\0';
	cfg_sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	cfg_sta.sta.pmf_cfg.capable = true;
	cfg_sta.sta.pmf_cfg.required = false;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg_sta));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "WiFi STA initialized. Connecting to SSID: %s", ssid);
}

WiFiSTA::~WiFiSTA() {
	esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEvtHandler);
	esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ipEvtHandler);
	esp_wifi_stop();
	esp_wifi_deinit();
}

bool WiFiSTA::isConnected() const {
	return connected;
}

std::string WiFiSTA::getIP() const {
	if (netif == nullptr) {
		return "0.0.0.0";
	}

	esp_netif_ip_info_t ip_info;
	if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
		return "0.0.0.0";
	}

	char ip_str[16];
	snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
	return std::string(ip_str);
}

void WiFiSTA::event(esp_event_base_t base, int32_t id, void* data) {
	if (base == WIFI_EVENT) {
		if (id == WIFI_EVENT_STA_START) {
			ESP_LOGI(TAG, "WiFi STA started, attempting to connect...");
			esp_wifi_connect();
		} else if (id == WIFI_EVENT_STA_CONNECTED) {
			ESP_LOGI(TAG, "Connected to AP");
			connected = true;
			
			Event evt { .action = Event::Connect };
			Events::post(Facility::WiFi, evt);
		} else if (id == WIFI_EVENT_STA_DISCONNECTED) {
			auto event = (wifi_event_sta_disconnected_t*) data;
			ESP_LOGI(TAG, "Disconnected from AP, reason: %d. Reconnecting...", event->reason);
			connected = false;
			
			Event evt { .action = Event::Disconnect };
			Events::post(Facility::WiFi, evt);
			
			esp_wifi_connect();
		}
	} else if (base == IP_EVENT) {
		if (id == IP_EVENT_STA_GOT_IP) {
			auto event = (ip_event_got_ip_t*) data;
			ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
			
			Event evt { .action = Event::GotIP };
			evt.gotIP.ip = event->ip_info.ip.addr;
			evt.gotIP.netmask = event->ip_info.netmask.addr;
			evt.gotIP.gw = event->ip_info.gw.addr;
			Events::post(Facility::WiFi, evt);
		}
	}
}

void WiFiSTA::createNetif() {
	netif = esp_netif_create_default_wifi_sta();
	if (netif == nullptr) {
		ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
		ESP_ERROR_CHECK(ESP_FAIL);
	}
}
