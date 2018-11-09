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

#include "candy/core/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/ConflictAnalysis.h"

#include <candy/simp/SimpSolver.h>
#include <candy/rsil/BranchingHeuristics.h>
#include <candy/gates/MiterDetector.h>
#include <candy/utils/Runtime.h>
#include <candy/rsar/Heuristics.h>

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

    class RSILSolverBuilder {
    public:
        /// RSIL Common: Iff true, conjectures about backbone variables are taken into consideration for implicit learning.
        bool backbonesEnabled;

        /// RSIL Common: Iff true, the RSARHeuristic given by this parameter struct is used to filter the conjectures
        /// before running RSIL.
        bool filterByRSARHeuristic;

        /// RSIL Common: An RSARHeuristic used to filter the conjectures before running RSIL. May contain nullptr if
        /// filterByRSARHeuristic == false.
        std::shared_ptr<RefinementHeuristic> RSARHeuristic;

        /// RSIL Common: Iff true, the RSARHeuristic is only used to filter backbone conjectures.
        bool filterOnlyBackbones;

        /// RSIL Vanishing: The intervention probability half-life.
        uint64_t probHalfLife;

        /// RSIL Budget: The initial implication budget.
        uint64_t initialBudget;

        /// The conjecures to be used by this conflict seeking branching heuristic
        Conjectures conjectures;

        /// heuristic selection
        RSILMode mode;
        unsigned int size;

        RSILSolverBuilder() :
               backbonesEnabled(false),
               filterByRSARHeuristic(false),
               RSARHeuristic(nullptr),
               filterOnlyBackbones(false),
               probHalfLife(100000ull),
               initialBudget(10000ull),
               conjectures(),
               mode(RSILMode::UNRESTRICTED),
               size(3) { }
        ~RSILSolverBuilder() { }

        RSILSolverBuilder& withFilter(GateAnalyzer& analyzer, int maxInputs_ = -1)  {
            auto maxInputs = static_cast<unsigned long>(maxInputs_ == -1 ? analyzer.getProblem().nVars() : maxInputs_);
            auto heuristic = createInputDepCountRefinementHeuristic(analyzer, {maxInputs, 0});
            heuristic->beginRefinementStep();
            this->RSARHeuristic = shared_ptr<RefinementHeuristic>(heuristic.release());
            this->filterByRSARHeuristic = true;
            return *this;
        }

        RSILSolverBuilder& withArgs(const RSILArguments& rsilArgs) {
            this->filterByRSARHeuristic = rsilArgs.filterByInputDependencies;
            this->filterOnlyBackbones = rsilArgs.filterOnlyBackbones;
            this->RSARHeuristic = nullptr;
            this->initialBudget = rsilArgs.impbudget_initialBudget;
            this->probHalfLife = rsilArgs.vanishing_probabilityHalfLife;
            return *this;
        }

        RSILSolverBuilder& withConjectures(Conjectures& conjectures) {
            this->conjectures = std::move(conjectures);
            this->backbonesEnabled = !conjectures.getBackbones().empty();
            return *this;
        }

        RSILSolverBuilder& withMode(RSILMode mode) {
            this->mode = mode;
            return *this;
        }

        RSILSolverBuilder& withSize(unsigned int size) {
            this->size = size;
            return *this;
        }


        //        if (generalArgs.rsilArgs.filterByRSARHeuristic) {
        //            std::unique_ptr<GateAnalyzer> gateAnalyzer = createGateAnalyzer(problem);
        //            builder = builder.withFilter(*gateAnalyzer.get());
        //        }

        CandySolverInterface* build() {
            ClauseDatabase* clause_db = new ClauseDatabase();
            Trail* trail = new Trail();
            Propagate* propagator = new Propagate(*trail);
	        ConflictAnalysis* conflict_analysis = new ConflictAnalysis(*trail, *propagator);
            if (size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    RSILBranchingHeuristic3* branch = new RSILBranchingHeuristic3(*trail, *conflict_analysis, std::move(conjectures), backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones); 
                    return new Solver<ClauseDatabase, Trail, Propagate, ConflictAnalysis, RSILBranchingHeuristic3>(*clause_db, *trail, *propagator, *conflict_analysis, *branch);
                }
                else if (mode == RSILMode::VANISHING) {
                    RSILVanishingBranchingHeuristic3* branch = new RSILVanishingBranchingHeuristic3(*trail, *conflict_analysis, std::move(conjectures), backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones, probHalfLife);
                    return new Solver<ClauseDatabase, Trail, Propagate, ConflictAnalysis, RSILVanishingBranchingHeuristic3>(*clause_db, *trail, *propagator, *conflict_analysis, *branch);
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    RSILBudgetBranchingHeuristic3* branch = new RSILBudgetBranchingHeuristic3(*trail, *conflict_analysis, std::move(conjectures), backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones, initialBudget);
                    return new Solver<ClauseDatabase, Trail, Propagate, ConflictAnalysis, RSILBudgetBranchingHeuristic3>(*clause_db, *trail, *propagator, *conflict_analysis, *branch);
                }
            }
            else if (size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    RSILBranchingHeuristic2* branch = new RSILBranchingHeuristic2(*trail, *conflict_analysis, std::move(conjectures), backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones); 
                    return new Solver<ClauseDatabase, Trail, Propagate, ConflictAnalysis, RSILBranchingHeuristic2>(*clause_db, *trail, *propagator, *conflict_analysis, *branch);
                }
                else if (mode == RSILMode::VANISHING) {
                    RSILVanishingBranchingHeuristic2* branch = new RSILVanishingBranchingHeuristic2(*trail, *conflict_analysis, std::move(conjectures), backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones, probHalfLife);
                    return new Solver<ClauseDatabase, Trail, Propagate, ConflictAnalysis, RSILVanishingBranchingHeuristic2>(*clause_db, *trail, *propagator, *conflict_analysis, *branch);
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    RSILBudgetBranchingHeuristic2* branch = new RSILBudgetBranchingHeuristic2(*trail, *conflict_analysis, std::move(conjectures), backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones, initialBudget);
                    return new Solver<ClauseDatabase, Trail, Propagate, ConflictAnalysis, RSILBudgetBranchingHeuristic2>(*clause_db, *trail, *propagator, *conflict_analysis, *branch);
                }
            }
            throw std::invalid_argument{"RSIL mode not implemented"};
		}
    };
}

#endif
