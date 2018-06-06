/*
 * Sonification.h
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#ifndef SRC_SONIFICATION_SONIFICATION_H_
#define SRC_SONIFICATION_SONIFICATION_H_

#ifdef SONIFICATION
#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>
#include <chrono>
#endif

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 7000

#define OUTPUT_BUFFER_SIZE 2048

class Sonification {
private:
#ifdef SONIFICATION
    UdpTransmitSocket transmitSocket;
    std::chrono::system_clock::time_point scheduleOffset;
#else
    void* transmitSocket;
    void* scheduleOffset;
#endif

public:
	Sonification();
	Sonification(int port);
	Sonification(const char* address, int port);

	virtual ~Sonification();

	void sendNumber(const char* name, int value);
	void scheduleSendNumber(const char* name, int value, int delay = 1);
};

#endif /* SRC_SONIFICATION_SONIFICATION_H_ */
