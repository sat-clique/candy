#include "minisat/core/Solver.h"
#include "util.h"

using namespace Candy;

extern lbool minisat_result(CNFProblem const &problem) {
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
