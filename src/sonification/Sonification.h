/*
 * Sonification.h
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#ifndef SRC_SONIFICATION_SONIFICATION_H_
#define SRC_SONIFICATION_SONIFICATION_H_

#ifdef SONIFICATION
#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/ip/UdpSocket.h"
#endif

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 7000

#define OUTPUT_BUFFER_SIZE 2048

class Sonification {
private:
#ifdef SONIFICATION
    UdpTransmitSocket transmitSocket;
#else
    void* transmitSocket;
#endif

public:
	Sonification();
	Sonification(int port);
	Sonification(const char* address, int port);

	virtual ~Sonification();

	void sendNumber(const char* name, int value);
};

#endif /* SRC_SONIFICATION_SONIFICATION_H_ */
