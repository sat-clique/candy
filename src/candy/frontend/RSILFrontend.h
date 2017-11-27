/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#ifndef X_D4B34ED5_A9FD_4916_A6D9_928B6FA7DDEE_H
#define X_D4B34ED5_A9FD_4916_A6D9_928B6FA7DDEE_H

#include <cstdint>
#include <iostream>

#include <candy/simp/SimpSolver.h>
#include <candy/rsil/BranchingHeuristics.h>
#include <candy/gates/MiterDetector.h>
#include <candy/utils/Runtime.h>

#include "Exceptions.h"
#include "RandomSimulationFrontend.h"
#include "GateAnalyzerFrontend.h"

namespace Candy {
    enum class RSILMode {
        UNRESTRICTED, ///< use RSIL for all decisions
        VANISHING, ///< use RSIL with decreasing probability (note that the pseudorandom number generator is determinized)
        IMPLICATIONBUDGETED ///< use RSIL with implication budgets
    };
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief RSIL argument structure
     */
    struct RSILArguments {
        /// iff true, RSIL should be used for SAT solving.
        const bool useRSIL;
        
        /// the RSIL mode to be used for RSIL-enabled SAT solving.
        const RSILMode mode;
        
        /// the half-life of the probability of performing decisions using RSIL (used only when mode=VANISHING)
        const uint64_t vanishing_probabilityHalfLife;
        
        /// the initial implication budgets (used only when mode=IMPLICATIONBUDGETED)
        const uint64_t impbudget_initialBudget;
        
        /// iff true, the conjectures about the problem to be solved are filtered by input dependency count.
        const bool filterByInputDependencies;
        
        /// the max. allowed input dependency count of literals in conjectures (used only when filterByInputDependencies == true)
        const int filterByInputDependenciesMax;
        
        /// restrict input-dependency-filtering to conjectures about backbone variables
        const bool filterOnlyBackbones;
        
        /// the minimal fraction of gate outputs (wrt. the total amount of variables of the problem) for RSIL to be enabled.
        const double minGateOutputFraction;
        
        /// iff true, the problem to be solved is deemed unsuitable for RSIL if it is not (heuristically) recognized as a miter.
        const bool useRSILOnlyForMiters;
    };
    
    std::ostream& operator <<(std::ostream& stream, const RSILArguments& arguments);
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Parses strings representing RSIL modes.
     *
     * \param mode      The RSIL mode. Must be one of unrestricted, implicationbudgeted, or
     *                  vanishing.
     *
     * \returns the corresponding RSILMode value.
     */
    RSILMode getRSILMode(const std::string& mode);

    template<class BranchingHeuristic>
    class RSILSolverFactory {
    public:
    	RSILSolverFactory() { }
    	~RSILSolverFactory() { }

    	RSILBranchingHeuristic3::Parameters getDefaultHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer);

		typename BranchingHeuristic::Parameters getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer);

		SimpSolver<BranchingHeuristic>*
		createRSILSolver(const GateRecognitionArguments& gateRecognitionArgs,
				const RandomSimulationArguments& randomSimulationArgs,
				const RSILArguments& rsilArgs,
				CNFProblem& problem) {
			Runtime runtime;
			GateRecognitionArguments localGateRecognitionArgs = gateRecognitionArgs;
			if (randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{-1}) {
				runtime.setTimeout(randomSimulationArgs.preprocessingTimeLimit);
				localGateRecognitionArgs.opt_gr_timeout = std::min(gateRecognitionArgs.opt_gr_timeout,
																   randomSimulationArgs.preprocessingTimeLimit);
			}
			runtime.start();

			auto analyzer = createGateAnalyzer(problem, localGateRecognitionArgs);
			analyzer->analyze();
			std::cerr << "c Gate recognition time: " << runtime.lap().count() << " ms" << std::endl;

			double problemVars = problem.nVars();
			double gateOutputs = analyzer->getGateCount();

			if (analyzer->hasTimeout() || runtime.hasTimeout()) {
				// Abort RSIL, since the probability is too high that this run
				// is not reproducible if we continue
				throw UnsuitableProblemException{"gate analysis exceeded the preprocessing time limit."};
			}

			if (problemVars == 0 || (gateOutputs/problemVars) < rsilArgs.minGateOutputFraction) {
				std::string errorMessage = std::string{"insufficient gate count "} + std::to_string(analyzer->getGateCount())
				+ std::string{"/"} + std::to_string(problem.nVars()) + std::string{"."};
				throw UnsuitableProblemException{errorMessage};
			}

			if (rsilArgs.useRSILOnlyForMiters) {
				bool isMiter = hasPossiblyMiterStructure(*analyzer);
				std::cerr << "c Miter recognition time: " << runtime.lap().count() << " ms" << std::endl;

				if (runtime.hasTimeout()) {
					throw UnsuitableProblemException{"miter detection exceeded the preprocessing time limit."};
				}
				if (!isMiter) {
					throw UnsuitableProblemException{"problem heuristically determined not to be a miter problem."};
				}
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

			auto heuristicParameters = getRSILHeuristicParameters(*conjectures, rsilArgs, *analyzer);
			return new SimpSolver<BranchingHeuristic>(heuristicParameters);
		}
    };
}

#endif
