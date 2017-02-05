/*
 * SolverSonification.cc
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#include <sonification/SolverSonification.h>

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

void SolverSonification::decisionLevel(int level) {
  sendNumber("/decision", level);
}

void SolverSonification::backtrackLevel(int level) {
  sendNumber("/backtrack", level);
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
