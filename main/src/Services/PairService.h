#ifndef PERSE_ROVER_PAIRSERVICE_H
#define PERSE_ROVER_PAIRSERVICE_H

#include "Periph/WiFiSTA.h"
#include "TCPServer.h"
#include "Util/Threaded.h"

class PairService : private Threaded {
public:
	PairService();
	~PairService() override;

	struct Event {
		bool success;
	};

	enum class State{
		Pairing, Success
	};
	State getState() const;

private:
	WiFiSTA& wifi;
	TCPServer& tcp;

	State state = State::Pairing;

	void loop() override;
};


#endif //PERSE_ROVER_PAIRSERVICE_H
