/*
 * SolverSonification.h
 *
 *  Created on: Jan 30, 2017
 *      Author: markus
 */

#ifndef SRC_SONIFICATION_SOLVERSONIFICATION_H_
#define SRC_SONIFICATION_SOLVERSONIFICATION_H_

#include <sonification/Sonification.h>

class SolverSonification: public Sonification {
public:
	SolverSonification();
	SolverSonification(int port);
	SolverSonification(const char* address, int port);
	virtual ~SolverSonification();

	void start(int nVars, int nClauses);
	void stop(int sat);
	void restart();
	void decisionLevel(int level);
	void conflictLevel(int level);
	void assignmentLevel(int level);
	void learntSize(int size);
};

#endif /* SRC_SONIFICATION_SOLVERSONIFICATION_H_ */
