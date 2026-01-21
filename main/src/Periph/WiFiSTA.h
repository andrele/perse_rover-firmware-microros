#ifndef PERSE_ROVER_WIFISTA_H
#define PERSE_ROVER_WIFISTA_H

#include <esp_event.h>
#include <esp_netif_types.h>
#include <string>

class WiFiSTA {
public:
	WiFiSTA(const char* ssid, const char* password);
	~WiFiSTA();

	struct Event {
		enum { Connect, Disconnect, GotIP } action;
		union {
			struct {
				uint32_t ip;
				uint32_t netmask;
				uint32_t gw;
			} gotIP;
		};
	};

	bool isConnected() const;
	std::string getIP() const;

private:
	esp_event_handler_instance_t wifiEvtHandler;
	esp_event_handler_instance_t ipEvtHandler;
	esp_netif_t* netif;
	void event(esp_event_base_t base, int32_t id, void* data);

	void createNetif();
	
	bool connected;
};


#endif //PERSE_ROVER_WIFISTA_H
