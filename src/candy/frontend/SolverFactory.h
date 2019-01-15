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

#include "candy/rsar/ARSolver.h" 
#include "candy/utils/StringUtils.h" 
#include "candy/frontend/RandomSimulationFrontend.h"

namespace Candy {

SimplificationHandlingMode parseSimplificationHandlingMode(const std::string& str) {
	if (str == "DISABLE") {
		return Candy::SimplificationHandlingMode::DISABLE;
	}
	if (str == "FREEZE") {
		return Candy::SimplificationHandlingMode::FREEZE;
	}
	if (str == "RESTRICT") {
		return Candy::SimplificationHandlingMode::RESTRICT;
	}
	if (str == "FULL") {
		return Candy::SimplificationHandlingMode::FULL;
	}
	throw std::invalid_argument(str + ": Unknown simplification handling mode");
}

CandySolverInterface* createRSARSolver(CNFProblem& problem) {
	// std::chrono::milliseconds startCPUTime = cpuTime();
	bool initializationComleted = false;
	std::unique_ptr<Conjectures> conjectures;
	
	GateAnalyzer analyzer { problem };
	analyzer.analyze();

	if (analyzer.hasTimeout()) {
		std::cout << "c gate analysis exceeded the preprocessing time limit." << std::endl;
	}
	else if (analyzer.getGateCount() < RSAROptions::opt_rsar_minGateCount) {
		std::cout << "c insufficient gate count." << std::endl;
	}
	else {
		try {
			conjectures = performRandomSimulation(analyzer, std::chrono::milliseconds(RandomSimulationOptions::opt_rs_ppTimeLimit));
			if (conjectures->getEquivalences().empty() && conjectures->getBackbones().empty()) {
				std::cout << "c no conjectures found." << std::endl;
			} else {
				initializationComleted = true;
			}
		}
		catch(OutOfTimeException& exception) {
			std::cout << "c random simulation exceeded the preprocessing time limit." << std::endl;
		}
	}

	if (initializationComleted) {
		auto arSolverBuilder = Candy::createARSolverBuilder();
		arSolverBuilder->withConjectures(std::move(conjectures));
		arSolverBuilder->withMaxRefinementSteps(RSAROptions::opt_rsar_maxRefinementSteps);
		arSolverBuilder->withSimplificationHandlingMode(parseSimplificationHandlingMode(std::string { RSAROptions::opt_rsar_simpMode }));

		if (RSAROptions::opt_rsar_inputDepCountHeurConf > 0) {
			std::vector<size_t> limits = tokenizeByWhitespace<size_t>(std::string { RSAROptions::opt_rsar_inputDepCountHeurConf }); 
			if (limits.size() == 0) {
				throw std::invalid_argument("invalid limits");
			}
			arSolverBuilder->addRefinementHeuristic(createInputDepCountRefinementHeuristic(analyzer, limits));
		}

		arSolverBuilder->withSolver(new Solver<>());

		return arSolverBuilder->build();
	}
	else {
		return nullptr;
	}
}

}

#endif /* SRC_CANDY_FRONTEND_SOLVERFACTORY_H_ */
