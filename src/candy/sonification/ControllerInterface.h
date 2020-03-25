/*************************************************************************************************
Candy -- Copyright (c) 2015-2020, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_SONIFICATION_CONTROLLERINTERFACE_H_
#define SRC_CANDY_SONIFICATION_CONTROLLERINTERFACE_H_

#include <iostream>
#include <thread>

#ifdef SONIFICATION
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "ip/UdpSocket.h"

#define DEFAULT_LISTEN_ADDRESS "127.0.0.1"
#define DEFAULT_LISTEN_PORT 7001

class ControllerInterface : public osc::OscPacketListener {

	ControllerInterface* listener;
	UdpListeningReceiveSocket* s;

	std::thread* t;

public:
	ControllerInterface() {}
	virtual ~ControllerInterface() {}

	void thread_run(UdpListeningReceiveSocket* s) {
		s->Run();
	}

	void run() {
		s = new UdpListeningReceiveSocket(IpEndpointName(DEFAULT_LISTEN_ADDRESS, DEFAULT_LISTEN_PORT), this);
		t = new std::thread(&ControllerInterface::thread_run, this, s);
	}

protected:
	virtual void ProcessMessage(const osc::ReceivedMessage& m, const IpEndpointName& remoteEndpoint) override {
		try {
			// example of parsing single messages. osc::OsckPacketListener
			// handles the bundle traversal.
			if (strcmp(m.AddressPattern(), "/test1") == 0) {
				// example #1 -- argument stream interface
				osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
				bool a1;
				osc::int32 a2;
				float a3;
				const char *a4;
				args >> a1 >> a2 >> a3 >> a4 >> osc::EndMessage;

				std::cout << "received '/test1' message with arguments: " << a1 << " " << a2 << " " << a3 << " " << a4 << "\n";
			}
			else if (strcmp(m.AddressPattern(), "/test2") == 0) {
				// example #2 -- argument iterator interface, supports
				// reflection for overloaded messages (eg you can call
				// (*arg)->IsBool() to check if a bool was passed etc).
				osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
				bool a1 = (arg++)->AsBool();
				int a2 = (arg++)->AsInt32();
				float a3 = (arg++)->AsFloat();
				const char *a4 = (arg++)->AsString();
				if (arg != m.ArgumentsEnd())
					throw osc::ExcessArgumentException();

				std::cout << "received '/test2' message with arguments: " << a1 << " " << a2 << " " << a3 << " " << a4 << "\n";
			}
		}
		catch (osc::Exception& e) {
			// any parsing errors such as unexpected argument types, or
			// missing arguments get thrown as exceptions.
			std::cout << "error while parsing message: " << m.AddressPattern() << ": " << e.what() << "\n";
		}
	}

};
#else
class ControllerInterface {
public:
    ControllerInterface();
    virtual ~ControllerInterface();
    void run() { }
};

#endif

#endif /* SRC_CANDY_SONIFICATION_CONTROLLERINTERFACE_H_ */

