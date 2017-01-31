/*
 * Sonification.cc
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#include <src/sonification/Sonification.h>

#ifdef SONIFICATION
Sonification::Sonification() :
	transmitSocket(IpEndpointName(DEFAULT_ADDRESS, DEFAULT_PORT))
{
}

Sonification::Sonification(int port) :
	transmitSocket(IpEndpointName(DEFAULT_ADDRESS, port))
{
}

Sonification::Sonification(const char* address, int port) :
	transmitSocket(IpEndpointName(address, port))
{
}
#else
Sonification::Sonification() : transmitSocket(nullptr) { }
Sonification::Sonification(int port) : transmitSocket(nullptr) { }
Sonification::Sonification(const char* address, int port) : transmitSocket(nullptr) { }
#endif

Sonification::~Sonification() { }

void Sonification::sendNumber(const char* name, int value) {
#ifdef SONIFICATION
  char buffer[OUTPUT_BUFFER_SIZE];
  osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

  p << osc::BeginBundleImmediate
      << osc::BeginMessage( name )
          << value << osc::EndMessage
      << osc::EndBundle;

  transmitSocket.Send(p.Data(), p.Size());
#endif
}

