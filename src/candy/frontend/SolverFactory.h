/*
 * SolverFactory.h
 *
 *  Created on: Dec 1, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_FRONTEND_SOLVERFACTORY_H_
#define SRC_CANDY_FRONTEND_SOLVERFACTORY_H_

#include "candy/core/CNFProblem.h"
#include "candy/core/CandySolverInterface.h"

#include "candy/rsil/BranchingHeuristics.h"

// Argument Structs Definitions
#include "candy/frontend/CandyCommandLineParser.h"
#include "candy/frontend/GateAnalyzerFrontend.h"
#include "candy/frontend/RandomSimulationFrontend.h"
#include "candy/frontend/RSARFrontend.h"
#include "candy/frontend/RSILFrontend.h"

namespace Candy {

class SolverFactory {
private:
	const GlucoseArguments& generalArgs;
	const GateRecognitionArguments& gateRecognitionArgs;
	const RandomSimulationArguments& randomSimulationArgs;
	const RSARArguments& rsarArgs;
	const RSILArguments& rsilArgs;

public:
	SolverFactory(GlucoseArguments& args) :
		generalArgs(args),
		gateRecognitionArgs(args.gateRecognitionArgs),
		randomSimulationArgs(args.randomSimulationArgs),
		rsarArgs(args.rsarArgs),
		rsilArgs(args.rsilArgs)
	{ }

	virtual ~SolverFactory() {}

	CandySolverInterface* createSolver(CNFProblem& problem);
	CandySolverInterface* createSimpSolver(CNFProblem& problem);
	CandySolverInterface* createRSILSolver(CNFProblem& problem);
	CandySolverInterface* createRSARSolver(CNFProblem& problem);

};

}

#endif /* SRC_CANDY_FRONTEND_SOLVERFACTORY_H_ */
