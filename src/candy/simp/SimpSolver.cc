/***************************************************************************************[SimpSolver.cc]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 LRI  - Univ. Paris Sud, France (2009-2013)
 Labri - Univ. Bordeaux, France

 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 Labri - Univ. Bordeaux, France

 Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
 Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it
 is based on. (see below).

 Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
 version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
 without restriction, including the rights to use, copy, modify, merge, publish, distribute,
 sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 - The above and below copyrights notices and this permission notice shall be included in all
 copies or substantial portions of the Software;
 - The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
 be used in any competitive event (sat competitions/evaluations) without the express permission of
 the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
 using Glucose Parallel as an embedded SAT engine (single core or not).


 --------------- Original Minisat Copyrights

 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson

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

#include <candy/simp/SimpSolver.h>
#include <candy/utils/System.h>
#include <candy/rsil/BranchingHeuristics.h>

using namespace Candy;
using namespace Glucose;
using namespace std;

//=================================================================================================
// Options:

namespace Candy {
namespace SimpSolverOptions {
const char* _cat = "SIMP";

BoolOption opt_use_asymm(_cat, "asymm", "Shrink clauses by asymmetric branching.", false);
BoolOption opt_use_rcheck(_cat, "rcheck", "Check if a clause is already implied. (costly)", false);
BoolOption opt_use_elim(_cat, "elim", "Perform variable elimination.", true);
IntOption opt_grow(_cat, "grow", "Allow a variable elimination step to grow by a number of clauses.", 0);
IntOption opt_clause_lim(_cat, "cl-lim", "Variables are not eliminated if it produces a resolvent with a length above this limit.", 20, IntRange(0, INT32_MAX));
}

namespace SimpSolverImpl {

void mkElimClause(vector<uint32_t>& elimclauses, Lit x) {
    elimclauses.push_back(toInt(x));
    elimclauses.push_back(1);
}

void mkElimClause(vector<uint32_t>& elimclauses, Var v, Clause& c) {
    assert(c.contains(v));
    uint32_t first = elimclauses.size();
    
    // Copy clause to elimclauses-vector
    for (Lit lit : c) {
        if (var(lit) != v || first == elimclauses.size()) {
            elimclauses.push_back(lit);
        } else {
            // Swap such that 'v' will occur first in the clause:
            elimclauses.push_back(elimclauses[first]);
            elimclauses[first] = lit;
        }
    }
    
    // Store the length of the clause last:
    elimclauses.push_back(c.size());
}
}

template<> SimpSolver<RSILBranchingHeuristic3>::SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_) : 
                Solver<RSILBranchingHeuristic3>(std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate),
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

template<> SimpSolver<RSILBudgetBranchingHeuristic3>::SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t initialBudget_) : 
                Solver<RSILBudgetBranchingHeuristic3>(std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, initialBudget_),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate), 
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

template<> SimpSolver<RSILVanishingBranchingHeuristic3>::SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t m_probHalfLife_) : 
                Solver<RSILVanishingBranchingHeuristic3>(std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, m_probHalfLife_),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate), 
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

template<> SimpSolver<RSILBranchingHeuristic2>::SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_) : 
                Solver<RSILBranchingHeuristic2>(std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate), 
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

template<> SimpSolver<RSILBudgetBranchingHeuristic2>::SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t initialBudget_) : 
                Solver<RSILBudgetBranchingHeuristic2>(std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, initialBudget_),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate), 
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

template<> SimpSolver<RSILVanishingBranchingHeuristic2>::SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t m_probHalfLife_) : 
                Solver<RSILVanishingBranchingHeuristic2>(std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, m_probHalfLife_),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate), 
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

}
