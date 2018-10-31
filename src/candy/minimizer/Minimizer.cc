/*
 * Minimize.cc
 *
 *  Created on: 10.01.2013
 *      Author: markus
 */

#include "candy/core/SolverTypes.h"
#include "candy/core/Solver.h"

#include "candy/gates/GateAnalyzer.h"

#include "candy/minimizer/Minimizer.h"

#define M_VERBOSE 0

namespace Candy {

/**
 * Maximize the number of don't cares in the given model relative to the given projection
 */
Minimizer::Minimizer(CNFProblem& _problem, Cl _model) : problem(_problem), model(_model), hittingSetProblem() { }

Minimizer::~Minimizer() { }

void Minimizer::generateHittingSetProblem(For& clauses) {
    for (Cl* clause : clauses) {
        // create clause containing all satified literals (purified)
        Cl normalizedClause;
        for (Lit lit : *clause) {
            if (model[var(lit)] == lit) {
                normalizedClause.push_back(mkLit(var(lit), false));
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
        For clauses = gateAnalyzer.getPrunedProblem(model);
        generateHittingSetProblem(clauses);
    }
    else {
        generateHittingSetProblem(problem.getProblem());
    }

    CandySolverInterface* solver = new Solver<VSIDS>();
    solver->addClauses(hittingSetProblem);
//    solver->printDIMACS();

    Cl normalizedModel;
    for (Lit lit : model) {
        normalizedModel.push_back(sign(lit) ? ~lit : lit); // add normalized
    }

    // find minimal model
    Cl minimizedModel = iterativeMinimization(solver, normalizedModel);

    // translate back to original variables
    Cl denormalizedModel;
    for (Lit lit : minimizedModel) {
        denormalizedModel.push_back(model[var(lit)]);
    }

    delete solver;

    return denormalizedModel;
}

Cl Minimizer::iterativeMinimization(CandySolverInterface* solver, Cl model) {
    Cl pModel(model.begin(), model.end());
    Cl exclude;
    Cl assume;

//    if (solver->nClauses() == 0)
//    exit(1);
#ifndef NDEBUG
    unsigned int i = 0;
#endif
    bool satisfiable = true;
    while (satisfiable) {
        exclude.clear();
        assume.clear();

        for (Lit lit : pModel) {
            if (sign(lit)) {
                assume.push_back(lit);
            }
            else {
                exclude.push_back(~lit);
            }
        }

//        printf("Model size: %zu\n", pModel.size());
//        printClause(pModel);
//        printf("Exclusion clause size: %zu\n", exclude.size());
//        printClause(exclude);
//        printf("New Assumptions: %zu\n", assume.size());
//        printClause(assume);
        assert(assume.size() >= i++);

        solver->addClause(exclude);

        if (solver->solve(assume) == l_True) {
            Cl newModel = solver->getModel();
            pModel.clear();
            pModel.insert(pModel.end(), newModel.begin(), newModel.end());
        }
        else {
            satisfiable = false;
        }
    }

    auto last = std::remove_if(pModel.begin(), pModel.end(), [](const Lit lit) { return sign(lit); });
    pModel.erase(last, pModel.end());

    return pModel;
}

}
