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
#include "candy/frontend/RSILSolverBuilder.h"

namespace Candy {

class SolverFactory {
private:
	const GlucoseArguments& generalArgs;
	const GateRecognitionArguments& gateRecognitionArgs;
	const RandomSimulationArguments& randomSimulationArgs;
	const RSARArguments& rsarArgs;
	const RSILArguments& rsilArgs;

    std::unique_ptr<Conjectures> generateConjectures(CNFProblem& problem);

public:
	SolverFactory(GlucoseArguments& args) :
		generalArgs(args),
		gateRecognitionArgs(args.gateRecognitionArgs),
		randomSimulationArgs(args.randomSimulationArgs),
		rsarArgs(args.rsarArgs),
		rsilArgs(args.rsilArgs)
	{ }

    ~SolverFactory() {}

	std::unique_ptr<GateAnalyzer> createGateAnalyzer(CNFProblem& problem);

	CandySolverInterface* createRSILSolver(CNFProblem& problem);
	CandySolverInterface* createRSARSolver(CNFProblem& problem);

};

}

#endif /* SRC_CANDY_FRONTEND_SOLVERFACTORY_H_ */
