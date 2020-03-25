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
#include <unistd.h>
#include <chrono>
#include <thread>
#endif

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
    int delay;

public:
#ifdef SONIFICATION
    Sonification(const char* address, int port, int delay_) :
        transmitSocket(IpEndpointName(address, port)), scheduleOffset(std::chrono::system_clock::now()), delay(delay_)
    { }
#else
    Sonification(const char* address, int port, int delay_) : 
        transmitSocket(nullptr), scheduleOffset(nullptr), delay(delay_) 
    { }
#endif

    virtual ~Sonification() { }

    void send(const char* name, int value, bool wait = false) {    
#ifdef SONIFICATION
    if (wait) {
        std::this_thread::sleep_until(scheduleOffset + std::chrono::milliseconds(delay));
        scheduleOffset = std::chrono::system_clock::now();
    }

    char buffer[OUTPUT_BUFFER_SIZE];
    osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

    p << osc::BeginBundleImmediate
        << osc::BeginMessage( name )
            << value << osc::EndMessage
        << osc::EndBundle;

    transmitSocket.Send(p.Data(), p.Size());
#endif
}

};

#endif /* SRC_SONIFICATION_SONIFICATION_H_ */
