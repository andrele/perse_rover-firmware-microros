#include "PairService.h"
#include "Util/Services.h"
#include "Util/Events.h"
#include "Util/stdafx.h"

PairService::PairService() : Threaded("PairService", 2 * 1024),
							 wifi(*(WiFiSTA*) Services.get(Service::WiFi)),
							 tcp(*(TCPServer*) Services.get(Service::TCP)){
	start();
}

PairService::~PairService(){
	stop();
}

void PairService::loop(){
	// Wait until WiFi is connected before accepting TCP connections
	if(!wifi.isConnected()){
		delayMillis(100);
		return;
	}

	const bool accepted = tcp.accept();

	if(accepted){
		state = State::Success;

		Event evt{ true };
		Events::post(Facility::Pair, evt);
		stop(0);

		return;
	}
	
	delayMillis(100);
}

PairService::State PairService::getState() const{
	return state;
}
