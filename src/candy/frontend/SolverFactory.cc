/*
 * SolverFactory.cc
 *
 *  Created on: Dec 1, 2017
 *      Author: markus
 */

#include "candy/frontend/SolverFactory.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/RSILSolverBuilder.h"
#include "candy/utils/MemUtils.h"
#include "candy/utils/StringUtils.h"
#include <candy/rsar/Heuristics.h>
#include <candy/rsil/BranchingHeuristics.h>

namespace Candy {

std::unique_ptr<GateAnalyzer> SolverFactory::createGateAnalyzer(CNFProblem& problem) {
    return backported_std::make_unique<Candy::GateAnalyzer>(problem,
                                                            generalArgs.gateRecognitionArgs.opt_gr_tries,
                                                            generalArgs.gateRecognitionArgs.opt_gr_patterns,
                                                            generalArgs.gateRecognitionArgs.opt_gr_semantic,
                                                            generalArgs.gateRecognitionArgs.opt_gr_holistic,
                                                            generalArgs.gateRecognitionArgs.opt_gr_lookahead,
                                                            generalArgs.gateRecognitionArgs.opt_gr_intensify,
                                                            generalArgs.gateRecognitionArgs.opt_gr_lookahead_threshold,
                                                            generalArgs.gateRecognitionArgs.opt_gr_semantic_budget,
                                                            generalArgs.gateRecognitionArgs.opt_gr_timeout);
}

std::unique_ptr<Conjectures> SolverFactory::generateConjectures(CNFProblem& problem) {
    Runtime runtime(randomSimulationArgs.preprocessingTimeLimit);
    runtime.start();

    auto analyzer = createGateAnalyzer(problem);
    analyzer->runtime.setTimeout(std::min(generalArgs.gateRecognitionArgs.opt_gr_timeout, generalArgs.randomSimulationArgs.preprocessingTimeLimit));
    analyzer->analyze();
    std::cerr << "c Gate recognition time: " << runtime.lap().count() << " ms" << std::endl;

    int problemVars = problem.nVars();
    int gateOutputs = analyzer->getGateCount();

    if (analyzer->hasTimeout() || runtime.hasTimeout()) {
        // Abort RSIL, since the probability is too high that this run is not reproducible if we continue
        throw UnsuitableProblemException{"gate analysis exceeded the preprocessing time limit."};
    }

    if (problemVars == 0 || (gateOutputs/problemVars) < rsilArgs.minGateOutputFraction) {
        std::string errorMessage = std::string{"insufficient gate count "} + std::to_string(analyzer->getGateCount())
        + std::string{"/"} + std::to_string(problem.nVars()) + std::string{"."};
        throw UnsuitableProblemException{errorMessage};
    }

    if (rsilArgs.useRSILOnlyForMiters && !hasPossiblyMiterStructure(*analyzer)) {
        throw UnsuitableProblemException{"problem heuristically determined not to be a miter problem."};
    }

    std::unique_ptr<Conjectures> conjectures;

    if (randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
        auto remainingTime = randomSimulationArgs.preprocessingTimeLimit - runtime.getRuntime();
        try {
            conjectures = performRandomSimulation(*analyzer, randomSimulationArgs, remainingTime);
        }
        catch(OutOfTimeException& exception) {
            std::cerr << "c Random simulation time: " << runtime.lap().count() << " ms" << std::endl;
            throw UnsuitableProblemException{"random simulation exceeded the preprocessing time limit."};
        }
    }
    else {
        conjectures = performRandomSimulation(*analyzer, randomSimulationArgs);
    }

    std::cerr << "c Random simulation time: " << runtime.lap().count() << " ms" << std::endl;

    if (conjectures->getEquivalences().empty() && conjectures->getBackbones().empty()) {
        throw UnsuitableProblemException{"no conjectures found."};
    }

    return conjectures;
}

CandySolverInterface* SolverFactory::createRSILSolver(CNFProblem& problem) {
    Conjectures& conjectures = *generateConjectures(problem).get();
    RSILSolverBuilder builder;
    builder.withArgs(generalArgs.rsilArgs);
    builder.withConjectures(conjectures);
    builder.withMode(generalArgs.rsilArgs.mode);
//		if (generalArgs.rsilArgs.filterByRSARHeuristic) {
//            std::unique_ptr<GateAnalyzer> gateAnalyzer = createGateAnalyzer(problem);
//            builder = builder.withFilter(*gateAnalyzer.get());
//		}
    return builder.build();
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

		arSolverBuilder->withSolver(new Solver<>());

		return arSolverBuilder->build();
	}
	catch (OutOfTimeException& e) {
		auto randomSimulationTime = cpuTime() - startCPUTime - gateAnalyzerTime;
		std::cerr << "c Random simulation time: " << randomSimulationTime.count() << " ms" << std::endl;

		throw UnsuitableProblemException{"Random simulation exceeded the time limit."};
	}
}

}
