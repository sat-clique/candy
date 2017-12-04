/*
 * SolverFactory.cc
 *
 *  Created on: Dec 1, 2017
 *      Author: markus
 */

#include "candy/frontend/SolverFactory.h"

namespace Candy {

CandySolverInterface* SolverFactory::createSolver(CNFProblem& problem) {
	return nullptr;
}

CandySolverInterface* SolverFactory::createSimpSolver(CNFProblem& problem) {
	return nullptr;
}

CandySolverInterface* SolverFactory::createRSILSolver(CNFProblem& problem) {
    if(generalArgs.rsilArgs.mode == RSILMode::UNRESTRICTED) {
        return (RSILSolverFactory<RSILBranchingHeuristic3>()).createRSILSolver(generalArgs.gateRecognitionArgs,
                generalArgs.randomSimulationArgs, generalArgs.rsilArgs, problem);
    }

//    if (generalArgs.rsilArgs.mode == RSILMode::VANISHING) {
//        return (RSILSolverFactory<RSILVanishingBranchingHeuristic3>()).createRSILSolver(generalArgs.gateRecognitionArgs,
//                generalArgs.randomSimulationArgs, generalArgs.rsilArgs, problem);
//    }

//    if (generalArgs.rsilArgs.mode == RSILMode::IMPLICATIONBUDGETED) {
//        return (RSILSolverFactory<RSILBudgetBranchingHeuristic3>()).createRSILSolver(generalArgs.gateRecognitionArgs,
//                generalArgs.randomSimulationArgs, generalArgs.rsilArgs, problem);
//    }

    throw std::invalid_argument{"RSIL mode not implemented"};
}

CandySolverInterface* SolverFactory::createRSARSolver(CNFProblem& problem) {
	return nullptr; //createARSolver(*gateAnalyzer, *solver, std::move(conjectures), rsarArgs);;
}

}
