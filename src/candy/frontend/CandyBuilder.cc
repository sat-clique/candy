/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include "candy/frontend/CandyBuilder.h"

#include "candy/core/CandySolverInterface.h"

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"

#include "candy/core/Solver.h"

namespace Candy {

enum class RSILMode {
    UNRESTRICTED, ///< use RSIL for all decisions
    VANISHING, ///< use RSIL with decreasing probability (note that the pseudorandom number generator is determinized)
    IMPLICATIONBUDGETED ///< use RSIL with implication budgets
};

RSILMode getRSILMode() {
    const std::string& mode = std::string{RSILOptions::opt_rsil_mode};
    if (mode == "unrestricted") {
        return RSILMode::UNRESTRICTED;
    }
    else if (mode == "vanishing") {
        return RSILMode::VANISHING;
    }
    else if (mode == "implicationbudgeted") {
        return RSILMode::IMPLICATIONBUDGETED;
    }
    else {
        throw std::invalid_argument("Error: unknown RSIL mode " + mode);
    }
}

template<class TPropagate, class TLearning, class TBranching> 
CandySolverInterface* CandyBuilder<TPropagate, TLearning, TBranching>::build() {
    return new Solver<TPropagate, TLearning, TBranching>();
}

CandySolverInterface* createSolver(bool staticPropagate, bool lrb, bool vsidsc, bool rsil, unsigned int rsil_adv_size) {
    CandyBuilder<> builder { }; 

    if (lrb) {
        if (staticPropagate) {
            std::cout << "c Building Solver of Type Solver<StaticPropagate, ConflictAnalysis, LRB>" << std::endl;
            return builder.branchWithLRB().propagateStaticClauses().build();
        } else {
            std::cout << "Building Solver of Type Solver<Propagate, ConflictAnalysis, LRB>" << std::endl;
            return builder.branchWithLRB().build();
        }
    } 
    else if (rsil) {
        RSILMode mode = getRSILMode();
        if (staticPropagate) {
            std::cout << "c Building Solver of Type Solver<StaticPropagate, ConflictAnalysis, RSIL>" << std::endl;
            if (rsil_adv_size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.propagateStaticClauses().branchWithRSILUnrestricted3().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.propagateStaticClauses().branchWithRSILVanishing3().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.propagateStaticClauses().branchWithRSILBudgeted3().build();
                }
            }
            else if (rsil_adv_size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.propagateStaticClauses().branchWithRSILUnrestricted2().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.propagateStaticClauses().branchWithRSILVanishing2().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.propagateStaticClauses().branchWithRSILBudgeted2().build();
                }
            }
        } else {
            std::cout << "c Building Solver of Type Solver<Propagate, ConflictAnalysis, RSIL>" << std::endl;
            if (rsil_adv_size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.branchWithRSILUnrestricted3().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.branchWithRSILVanishing3().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.branchWithRSILBudgeted3().build();
                }
            }
            else if (rsil_adv_size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.branchWithRSILUnrestricted2().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.branchWithRSILVanishing2().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.branchWithRSILBudgeted2().build();
                }
            }
        }
    }
    else if (vsidsc) {
       if (staticPropagate) {
            std::cout << "c Building Solver of Type Solver<StaticPropagate, ConflictAnalysis, VSIDSC>" << std::endl;
            return builder.branchWithVSIDSC().propagateStaticClauses().build();
        } else {
            std::cout << "c Building Solver of Type Solver<Propagate, ConflictAnalysis, VSIDSC>" << std::endl;
            return builder.branchWithVSIDSC().build();
        }
    }
    else {
        if (staticPropagate) {
            std::cout << "c Building Solver of Type Solver<StaticPropagate, ConflictAnalysis, VSIDS>" << std::endl;
            return builder.propagateStaticClauses().build();
        } else {
            std::cout << "c Building Solver of Type Solver<Propagate, ConflictAnalysis, VSIDS>" << std::endl;
            return builder.build();
        }
    }
    std::cout << "c Warning! Configuration Not Found. Building Solver of Type Solver<Propagate, ConflictAnalysis, VSIDS>" << std::endl;
    return builder.build();
}

}