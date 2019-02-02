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

#include <candy/sonification/SolverSonification.h>

SolverSonification::SolverSonification()
 : Sonification() {

}

SolverSonification::SolverSonification(int port)
 : Sonification(port) {

}

SolverSonification::SolverSonification(const char* address, int port)
 : Sonification(address, port) {

}

SolverSonification::~SolverSonification() { }


void SolverSonification::start(int nVars, int nClauses) {
  sendNumber("/start", 1);
  sendNumber("/variables", nVars);
  sendNumber("/clauses", nClauses);
}

void SolverSonification::stop(int sat) {
  sendNumber("/stop", sat);
}

void SolverSonification::restart() {
  sendNumber("/restart", 1);
}

void SolverSonification::decisionLevel(int level, int delay = 0) {
  scheduleSendNumber("/decision", level, delay);
}

void SolverSonification::conflictLevel(int level) {
  sendNumber("/conflict", level);
}

void SolverSonification::assignmentLevel(int level) {
  sendNumber("/assignments", level);
}

void SolverSonification::learntSize(int size) {
  sendNumber("/learnt", size);
}
