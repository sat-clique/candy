/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include <candy/sonification/Sonification.h>
#ifdef SONIFICATION
#include <unistd.h>
#include <chrono>
#include <thread>
#endif

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
