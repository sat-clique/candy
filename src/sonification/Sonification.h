/*
 * Sonification.h
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#ifndef SRC_SONIFICATION_SONIFICATION_H_
#define SRC_SONIFICATION_SONIFICATION_H_

#include "/usr/local/include/oscpack/osc/OscOutboundPacketStream.h"
#include "/usr/local/include/oscpack/ip/UdpSocket.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 7000

#define OUTPUT_BUFFER_SIZE 2048

class Sonification {
private:
    UdpTransmitSocket transmitSocket;

public:
	Sonification();
	Sonification(int port);
	Sonification(const char* address, int port);

	virtual ~Sonification();

	void sendNumber(const char* name, int value);
};

#endif /* SRC_SONIFICATION_SONIFICATION_H_ */
