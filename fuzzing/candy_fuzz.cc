#include "minisat/core/Solver.h"

#include "candy/core/Solver.h"
#include "candy/frontend/CandyCommandLineParser.h"
#include "candy/frontend/SolverFactory.h"
#include "candy/core/branching/VSIDS.h"

using namespace Candy;

lbool minisat_result(CNFProblem const &problem) {
    Minisat::Solver solver;
    for (unsigned int i = 0; i <= problem.nVars(); ++i) {
        solver.newVar();
    }

    for (auto* clause : problem.getProblem()) {
        Minisat::vec<Minisat::Lit> minisatClause;
        for (auto lit : *clause) {
            minisatClause.push(Minisat::mkLit(static_cast<Minisat::Var>(var(lit)), sign(lit)));
        }
        solver.addClause(minisatClause);
    }

    bool result = solver.solve();

    return result ? l_True : l_False;
}

int main(int argc, char** argv) {
    GlucoseArguments args = parseCommandLineArgs(argc, argv);

    CNFProblem problem{};
    if (args.read_from_stdin) {
        printf("c Reading from standard input... Use '--help' for help.\n");
        if (!problem.readDimacsFromStdin()) {
            return 1;
        }
    } else {
        if (!problem.readDimacsFromFile(args.input_filename)) {
            return 1;
        }
    }

    if (problem.nVars() > 100) {
        exit(3); // concentrate on small problems during fuzzing
    }

    CandySolverInterface* solver = new SimpSolver<VSIDS>();
    solver->addClauses(problem);

    lbool result = l_Undef;
    if (solver->isInConflictingState()) {
        result = l_False;
    }
    else {
        solver->eliminate(true, true);
        solver->disablePreprocessing();
        if (solver->isInConflictingState()) {
            result = l_False;
        }
        else {
            result = solver->solve();
        }
    }

    assert (result == minisat_result(problem));
}