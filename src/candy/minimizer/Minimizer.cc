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
Minimizer::Minimizer(CNFProblem& _problem, CandySolverResult& _model) : problem(_problem), model(_model) { 
    SolverOptions::opt_preprocessing = false;
    SolverOptions::opt_inprocessing = 0;
    SolverOptions::opt_sort_variables = false;
    solver = createSolver();
 }

Minimizer::~Minimizer() { 
    delete solver;
}


template<typename Iterator>
For Minimizer::purify(Iterator begin, Iterator end) {
    For result;
    for (auto it = begin; it < end; it++) {
        Cl* clause = *it;
        Cl* normalizedClause = new Cl();
        for (Lit lit : *clause) {
            if (model.value(lit.var()) == lit) {
                normalizedClause->push_back(Lit(lit.var(), false));
            }
        }
        result.push_back(normalizedClause);
    }
    return result;
}

void Minimizer::mimimizeModel(bool pruningActivated, bool computeMinimalModel, bool projectToInputs) {
    For hittingSetProblem;
    Cl minimizedModel;

    if (pruningActivated || projectToInputs) {
        GateAnalyzer gateAnalyzer(problem);
        gateAnalyzer.analyze();

        if (pruningActivated) {
            For clauses = gateAnalyzer.getGateProblem().getPrunedProblem(model);
            hittingSetProblem = purify(clauses.begin(), clauses.end());
        }
        else {
            hittingSetProblem = purify(problem.begin(), problem.end()); 
        }

        minimizedModel = iterativeMinimization(hittingSetProblem, computeMinimalModel);

        if (projectToInputs) {
            auto end = remove_if(minimizedModel.begin(), minimizedModel.end(), 
                [gateAnalyzer](Lit lit) { 
                    return gateAnalyzer.getGateProblem().isGateOutput(lit); 
                });
            minimizedModel.erase(end, minimizedModel.end());
        }
    }
    else {
        hittingSetProblem = purify(problem.begin(), problem.end());
        minimizedModel = iterativeMinimization(hittingSetProblem, computeMinimalModel);
    }

    // translate back to original polarity
    for (Lit lit : minimizedModel) {
        if (model.satisfies(lit)) {
            model.setMinimizedModelValue(lit);
        }
        else {
            model.setMinimizedModelValue(~lit);
        }
    }
}

Cl Minimizer::iterativeMinimization(For hittingSetProblem, bool computeMinimalModel) {
    Cl cardinality;
    Cl assumptions;

    solver->init(CNFProblem { hittingSetProblem });

    for (Var v = 0; v < (Var)this->problem.nVars(); v++) {
        cardinality.push_back(Lit(v, true));
    }

    lbool satisfiable = l_True;
    while (satisfiable == l_True) {
        solver->init(CNFProblem { cardinality });
        solver->getAssignment().setAssumptions(assumptions);

        satisfiable = solver->solve();

        if (satisfiable == l_True) {
            CandySolverResult result = solver->getCandySolverResult();

            cardinality.clear();
            assumptions.clear();
            for (Var v = 0; v < (Var)this->problem.nVars(); v++) {
                if (result.satisfies(Lit(v, false))) {
                    cardinality.push_back(Lit(v, true));
                }
                else {
                    assumptions.push_back(Lit(v, true));
                }
            }      
        }
    }

    if (computeMinimalModel) {
        CNFProblem counter, lessthan;
        Cl variables;
        for (Var v = 0; v < (Var)this->problem.nVars(); v++) {
            variables.push_back(Lit(v, false));
        }
        Cl outputs = genParCounter(counter, variables);
        solver->init(counter);
        satisfiable = l_True;
        while (satisfiable == l_True) {
            lessthan.clear();
            genLessThanConstraint(lessthan, outputs, cardinality.size());
            solver->init(lessthan);

            satisfiable = solver->solve();

            if (satisfiable == l_True) {
                CandySolverResult result = solver->getCandySolverResult();

                cardinality.clear();
                for (Var v = 0; v < (Var)this->problem.nVars(); v++) {
                    if (result.satisfies(Lit(v, false))) {
                        cardinality.push_back(Lit(v, true));
                    }
                }      
            }
        }
    }

    Cl result;
    for (Lit lit : cardinality) {
        result.push_back(~lit);
    }

    return result;
}

}
