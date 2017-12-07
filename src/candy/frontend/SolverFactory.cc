/*
 * SolverFactory.cc
 *
 *  Created on: Dec 1, 2017
 *      Author: markus
 */

#include "candy/frontend/SolverFactory.h"
#include "candy/utils/MemUtils.h"
#include "candy/utils/StringUtils.h"
#include <candy/rsar/Heuristics.h>

namespace Candy {

std::unique_ptr<GateAnalyzer> SolverFactory::createGateAnalyzer(CNFProblem& problem) {
    return backported_std::make_unique<Candy::GateAnalyzer>(problem,
                                                            gateRecognitionArgs.opt_gr_tries,
															gateRecognitionArgs.opt_gr_patterns,
															gateRecognitionArgs.opt_gr_semantic,
															gateRecognitionArgs.opt_gr_holistic,
															gateRecognitionArgs.opt_gr_lookahead,
															gateRecognitionArgs.opt_gr_intensify,
															gateRecognitionArgs.opt_gr_lookahead_threshold,
															gateRecognitionArgs.opt_gr_semantic_budget,
															gateRecognitionArgs.opt_gr_timeout);
}

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
	// TODO: the CPU time code was inserted in quite a hurry and
	// needs to be refactored.

	std::chrono::milliseconds startCPUTime = cpuTime();

	GateRecognitionArguments localGateRecognitionArgs = gateRecognitionArgs;
	if (generalArgs.randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
		localGateRecognitionArgs.opt_gr_timeout = std::min(gateRecognitionArgs.opt_gr_timeout,
				randomSimulationArgs.preprocessingTimeLimit);
	}

	std::unique_ptr<GateAnalyzer> gateAnalyzer = createGateAnalyzer(problem);
	gateAnalyzer->analyze();

	std::chrono::milliseconds gateAnalyzerTime = cpuTime() - startCPUTime;
	std::cerr << "c Gate recognition time: " << gateAnalyzerTime.count() << " ms" << std::endl;

	if (gateAnalyzer->hasTimeout()
		|| (randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{0}
			&& gateAnalyzerTime > randomSimulationArgs.preprocessingTimeLimit)) {
		throw UnsuitableProblemException{"Gate recognition exceeded the time limit."};
	}


	if (gateAnalyzer->getGateCount() < rsarArgs.minGateCount) {
		throw UnsuitableProblemException{std::string{"Insufficient gate count "}
			+ std::to_string(gateAnalyzer->getGateCount())};
	}

	std::chrono::milliseconds rsTimeLimit = std::chrono::milliseconds{-1};
	if (randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
		rsTimeLimit = randomSimulationArgs.preprocessingTimeLimit - gateAnalyzerTime;
	}

	try {
		auto conjectures = performRandomSimulation(*gateAnalyzer, randomSimulationArgs, rsTimeLimit);
		if (conjectures->getEquivalences().empty() && conjectures->getBackbones().empty()) {
			throw UnsuitableProblemException{"No conjectures found."};
		}

		auto randomSimulationTime = cpuTime() - startCPUTime - gateAnalyzerTime;
		std::cerr << "c Random simulation time: " << randomSimulationTime.count() << " ms" << std::endl;

		if (rsarArgs.simplificationHandlingMode == SimplificationHandlingMode::FREEZE) {
			throw std::runtime_error("The FREEZE simplification handling mode is temporarily unavailable");
		}

		auto arSolverBuilder = Candy::createARSolverBuilder();
		arSolverBuilder->withConjectures(std::move(conjectures));
		arSolverBuilder->withMaxRefinementSteps(rsarArgs.maxRefinementSteps);
		arSolverBuilder->withSimplificationHandlingMode(rsarArgs.simplificationHandlingMode);

		if (rsarArgs.withInputDepCountHeuristic) {
			 /*std::regex unsignedIntRegex { "^(\\s*[0-9]+\\s*)+$" };
			 if (!std::regex_match(limitsString, unsignedIntRegex)) {
			 throw std::invalid_argument(limitsString + ": invalid limits");
			 }
			 */
			std::vector<size_t> limits = Candy::tokenizeByWhitespace<size_t>(rsarArgs.inputDepCountHeuristicConfiguration);

			if (limits.size() == 0) {
				throw std::invalid_argument(rsarArgs.inputDepCountHeuristicConfiguration + ": invalid limits");
			}
			arSolverBuilder->addRefinementHeuristic(createInputDepCountRefinementHeuristic(*gateAnalyzer, limits));
		}

		arSolverBuilder->withSolver(new SimpSolver<Branch>());

		return arSolverBuilder->build();
	}
	catch (OutOfTimeException& e) {
		auto randomSimulationTime = cpuTime() - startCPUTime - gateAnalyzerTime;
		std::cerr << "c Random simulation time: " << randomSimulationTime.count() << " ms" << std::endl;

		throw UnsuitableProblemException{"Random simulation exceeded the time limit."};
	}
}

}
