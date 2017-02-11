/*
 * Sonification.cc
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#include <sonification/Sonification.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#ifdef SONIFICATION
Sonification::Sonification() :
	transmitSocket(IpEndpointName(DEFAULT_ADDRESS, DEFAULT_PORT)),
	scheduleOffset(std::chrono::system_clock::now())
{
}

Sonification::Sonification(int port) :
	transmitSocket(IpEndpointName(DEFAULT_ADDRESS, port)),
    scheduleOffset(std::chrono::system_clock::now())
{
}

Sonification::Sonification(const char* address, int port) :
	transmitSocket(IpEndpointName(address, port)),
    scheduleOffset(std::chrono::system_clock::now())
{
}
#else
Sonification::Sonification() : transmitSocket(nullptr), scheduleOffset(nullptr) { }
Sonification::Sonification(int port) : transmitSocket(nullptr), scheduleOffset(nullptr) { }
Sonification::Sonification(const char* address, int port) : transmitSocket(nullptr), scheduleOffset(nullptr) { }
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

void Sonification::scheduleSendNumber(const char* name, int value, int delay) {
#ifdef SONIFICATION
  std::this_thread::sleep_until(scheduleOffset + std::chrono::milliseconds(delay));
  scheduleOffset = std::chrono::system_clock::now();
  sendNumber(name, value);
#endif
}
