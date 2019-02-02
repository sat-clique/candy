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
