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
Minimizer::Minimizer(CNFProblem* problem) {
    this->clauses = problem;
}

Minimizer::~Minimizer() { }

CNFProblem* Minimizer::generateHittingSetProblem(For clauses, Cl model) {
    CNFProblem* normalizedProblem = new CNFProblem();
    for (Cl* clause : clauses) {
        // create clause containing all satified literals (purified)
        Cl normalizedClause;
        for (Lit lit : *clause) {
            if (model[var(lit)] == lit) {
                normalizedClause.push_back(mkLit(var(lit), false));
            }
        }

        normalizedProblem->readClause(normalizedClause);
    }

    return normalizedProblem;
}

Cl Minimizer::computeMinimalModel(Cl model, bool pruningActivated) {
    CNFProblem* normalizedClauses;

    if (pruningActivated) {
        GateAnalyzer gateAnalyzer(*clauses);
        gateAnalyzer.analyze();

        For newClauses = gateAnalyzer.getPrunedProblem(model);
        normalizedClauses = generateHittingSetProblem(newClauses, model);
    }
    else {
        normalizedClauses = generateHittingSetProblem(clauses->getProblem(), model);
    }

    CandySolverInterface* solver = new Solver<Branch>();
    solver->addClauses(*normalizedClauses);
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
    delete normalizedClauses;

    return denormalizedModel;
}

Cl Minimizer::iterativeMinimization(CandySolverInterface* solver, Cl model) {
    Cl pModel(model.begin(), model.end());
    Cl exclude;
    Cl assume;

//    if (solver->nClauses() == 0)
//    exit(1);

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

        satisfiable = solver->simplify() && solver->strengthen() && (solver->solve(assume) == l_True);

        if (satisfiable) {
            Cl newModel = solver->getModel();
            pModel.clear();
            pModel.insert(pModel.end(), newModel.begin(), newModel.end());
        }
    }

    auto last = std::remove_if(pModel.begin(), pModel.end(), [](const Lit lit) { return sign(lit); });
    pModel.erase(last, pModel.end());

    return pModel;
}

}
