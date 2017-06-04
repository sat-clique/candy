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
#include <candy/randomsimulation/RandomSimulator.h>

#include "Exceptions.h"
#include "RandomSimulationFrontend.h"
#include "GateAnalyzerFrontend.h"

namespace Candy {
    enum class RSILMode {
        UNRESTRICTED,
        VANISHING,
        IMPLICATIONBUDGETED
    };
    
    struct RSILArguments {
        const bool useRSIL;
        const RSILMode mode;
        const uint64_t vanishing_probabilityHalfLife;
        const uint64_t impbudget_initialBudget;
        
        const bool filterByInputDependencies;
        const int filterByInputDependenciesMax;
        const bool filterOnlyBackbones;
        const double minGateOutputFraction;
        
        const bool useRSILOnlyForMiters;
    };
    
    std::ostream& operator <<(std::ostream& stream, const RSILArguments& arguments);
    
    RSILMode getRSILMode(const std::string& mode);
    
    using RSILSolver = SimpSolver<RSILBranchingHeuristic2>;
    using RSILVanishingSolver = SimpSolver<RSILVanishingBranchingHeuristic2>;
    using RSILBugetSolver = SimpSolver<RSILBudgetBranchingHeuristic2>;
    
    template<class SolverType> static
    std::function<void(SolverType&, CNFProblem&)>
    createRSILPreprocessingHook(const GateRecognitionArguments& gateRecognitionArgs,
                                const RandomSimulationArguments& randomSimulationArgs,
                                const RSILArguments& rsilArgs);
    
    template<class PickBranchLitType> static
    typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILBranchingHeuristic3::BasicType>::value,
    typename PickBranchLitType::Parameters>::type
    getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer);
    
    template<class PickBranchLitType> static
    typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILVanishingBranchingHeuristic3::BasicType>::value,
    typename PickBranchLitType::Parameters>::type
    getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer);
    
    template<class PickBranchLitType> static
    typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILBudgetBranchingHeuristic3::BasicType>::value,
    typename PickBranchLitType::Parameters>::type
    getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer);
    
    
    
    /*********** Implementation ********************************************************/
    
    
    // getRSILHeuristicParameters is implemented using SFINAE.
    //
    // Solver<PickBranchLitType> exports the PickBranchLitType template argument as
    // Solver::PickBranchLitType, which in case of RSIL has a nested type
    // PickBranchLitType::BasicType (making life easier due to further template
    // arguments, such as the advice entry type. For each RSIL branching heuristic
    // type HEUR<T>, there is a type HEUR<T>::BasicType which is independent of T.
    // For example, we have
    // RSILBudgetBranchingHeuristic<3>::BasicType == RSILBudgetBranchingHeuristic<300>::BasicType
    //
    // As getRSILHeuristicParameters is independent of the heuristic template parameters,
    // getRSILHeuristicParameters is effectively specialized for the BasicType subtypes.
    // This is achieved via a common SFINAE pattern using std::enable_if and std::is_same:
    // if the std::is_same expression holds (i.e. std::is_same<...>::value is true), the
    // type std::enable_if<std::is_same<...>::value, T>::type is T, and otherwise
    // std::enable_if<std::is_same<...>::value, T>::type does not exist. (In this
    // case, T is the function's return value type.) The compiler uses the specialization
    // where std::enable_if<...>::type is defined.
    
    template<class PickBranchLitType> static
    typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILBranchingHeuristic3::BasicType>::value,
    typename PickBranchLitType::Parameters>::type
    getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer) {
        
        bool useBackbones = !conjectures.getBackbones().empty();
        std::shared_ptr<RefinementHeuristic> filterHeuristic = nullptr;
        
        if (rsilArgs.filterByInputDependencies) {
            auto maxInputs = static_cast<unsigned long>(rsilArgs.filterByInputDependenciesMax);
            auto heuristic = createInputDepCountRefinementHeuristic(analyzer, {maxInputs, 0});
            heuristic->beginRefinementStep();
            filterHeuristic = shared_ptr<RefinementHeuristic>(heuristic.release());
        }
        
        return typename PickBranchLitType::Parameters{conjectures,
            useBackbones,
            rsilArgs.filterByInputDependencies,
            filterHeuristic,
            rsilArgs.filterOnlyBackbones};
    }
    
    template<class PickBranchLitType> static
    typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILVanishingBranchingHeuristic3::BasicType>::value,
    typename PickBranchLitType::Parameters>::type
    getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer) {
        auto conf = getRSILHeuristicParameters<typename PickBranchLitType::UnderlyingHeuristicType>(conjectures,
                                                                                                    rsilArgs,
                                                                                                    analyzer);
        return {conf, rsilArgs.vanishing_probabilityHalfLife};
    }
    
    template<class PickBranchLitType> static
    typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILBudgetBranchingHeuristic3::BasicType>::value,
    typename PickBranchLitType::Parameters>::type
    getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer) {
        auto conf = getRSILHeuristicParameters<typename PickBranchLitType::UnderlyingHeuristicType>(conjectures,
                                                                                                    rsilArgs,
                                                                                                    analyzer);
        return {conf, rsilArgs.impbudget_initialBudget};
    }
    
    
    template<class SolverType> static
    std::function<void(SolverType&, CNFProblem&)>
    createRSILPreprocessingHook(const GateRecognitionArguments& gateRecognitionArgs,
                                const RandomSimulationArguments& randomSimulationArgs,
                                const RSILArguments& rsilArgs) {
        return [gateRecognitionArgs, randomSimulationArgs, rsilArgs](SolverType& solver, CNFProblem& problem) {
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
            
            auto heuristicParameters = getRSILHeuristicParameters<typename SolverType::PickBranchLitType>(*conjectures,
                                                                                                          rsilArgs,
                                                                                                          *analyzer);
            solver.initializePickBranchLit(heuristicParameters);
        };
    }

}

#endif
