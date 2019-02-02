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

#ifndef SRC_SONIFICATION_SOLVERSONIFICATION_H_
#define SRC_SONIFICATION_SOLVERSONIFICATION_H_

#include <candy/sonification/Sonification.h>

class SolverSonification: public Sonification {
public:
	SolverSonification();
	SolverSonification(int port);
	SolverSonification(const char* address, int port);
	virtual ~SolverSonification();

	void start(int nVars, int nClauses);
	void stop(int sat);
	void restart();
	void decisionLevel(int level, int delay);
	void conflictLevel(int level);
	void assignmentLevel(int level);
	void learntSize(int size);
};

#endif /* SRC_SONIFICATION_SOLVERSONIFICATION_H_ */
