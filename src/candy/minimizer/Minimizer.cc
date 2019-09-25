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

#include "candy/core/SolverTypes.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/CandySolverResult.h"
#include "candy/frontend/CandyBuilder.h"

#include "candy/gates/GateAnalyzer.h"

#include "candy/minimizer/Minimizer.h"

#define M_VERBOSE 0

namespace Candy {

/**
 * Maximize the number of don't cares in the given model relative to the given projection
 */
Minimizer::Minimizer(CNFProblem& _problem, Cl _model)
 : problem(_problem), model(_model), hittingSetProblem() { }

Minimizer::~Minimizer() { }

void Minimizer::generateHittingSetProblem(CNFProblem& clauses) {
    for (Cl* clause : clauses) {
        // create clause containing all satified literals (purified)
        Cl normalizedClause;
        for (Lit lit : *clause) {
            if (model[lit.var()] == lit) {
                normalizedClause.push_back(Lit(lit.var(), false));
            }
        }
        hittingSetProblem.readClause(normalizedClause);
    }
}

CNFProblem& Minimizer::getHittingSetProblem() {
    return hittingSetProblem;
}

Cl Minimizer::computeMinimalModel(bool pruningActivated) {
    if (pruningActivated) {
        GateAnalyzer gateAnalyzer(problem);
        gateAnalyzer.analyze();
        For clauses = gateAnalyzer.getGateProblem().getPrunedProblem(model);
        CNFProblem problem { clauses }; 
        generateHittingSetProblem(problem);
    }
    else {
        generateHittingSetProblem(problem);
    }

    SolverOptions::opt_preprocessing = false;
    CandySolverInterface* solver = createSolver();
    solver->init(hittingSetProblem);

    // compute minimal model for 
    hittingSetProblem.printDIMACS();

    Cl normalizedModel;
    for (Lit lit : model) {
        normalizedModel.push_back(lit.sign() ? ~lit : lit); // add normalized
    }

    // find minimal model
    Cl minimizedModel = iterativeMinimization(solver, normalizedModel);

    // translate back to original variables
    Cl denormalizedModel;
    for (Lit lit : minimizedModel) {
        denormalizedModel.push_back(model[lit.var()]);
    }

    delete solver;

    return denormalizedModel;
}

Cl Minimizer::iterativeMinimization(CandySolverInterface* solver, Cl model) {
    Cl minimize;
    Cl assumptions;

    for (Lit lit : model) {
        minimize.push_back(~lit);
    }

    lbool satisfiable = l_True;
    while (satisfiable == l_True) {
        solver->init(CNFProblem { minimize });
        solver->getAssignment().setAssumptions(assumptions);

        satisfiable = solver->solve();

        if (satisfiable == l_True) {
            CandySolverResult result = solver->getCandySolverResult();

            for (Lit lit : minimize) {
                if (result.modelValue(lit) == l_True) {
                    assumptions.push_back(lit);
                } 
            }

            minimize.clear();
            for (Lit lit : model) {
                if (result.modelValue(lit) == l_True) {
                    minimize.push_back(~lit);
                }
            }      
        }
    }

    Cl result;
    for (Lit lit : minimize) {
        result.push_back(~lit);
    }

    return result;
}

}
