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

#include "candy/core/CandySolverInterface.h"
#include "candy/core/CandySolverResult.h"
#include "candy/frontend/CandyBuilder.h"
#include "candy/core/SolverTypes.h"

#include <vector>

using namespace Candy;

class IPASIRCandy {

    CNFProblem problem;

    CandySolverInterface* solver;

    std::vector<Lit> assumptions;
    std::vector<Lit> clause;

    unsigned char* fmap;
    bool nomodel;

    Lit import(int lit) {
        return Lit(Var(abs(lit)-1), (lit < 0));
    }

    void analyze() {
        fmap = new unsigned char [solver->nVars()];
        memset(fmap, 0, solver->nVars());
        CandySolverResult& result = solver->getCandySolverResult();
        for (unsigned int i = 0; i < result.getConflict().size(); ++i) {
            fmap[result.getConflict()[i].var()] = 1;
        }
    }

    void drop_analysis() {
        if(fmap) delete [] fmap;
        fmap = 0;
    }

public:
    IPASIRCandy() : fmap(0), nomodel(false) { 
        solver = createSolver();
    }

    ~IPASIRCandy() {
        drop_analysis();
        delete solver;
    }

    void add(int lit) {
        drop_analysis();
        nomodel = true;
        if (lit) {
            clause.push_back(import(lit));
        } else {
            problem.readClause(clause.begin(), clause.end());
            clause.clear();
        }
    }

    void assume(int lit) {
        drop_analysis();
        nomodel = true;
        assumptions.push_back(import(lit));
    }

    int solve() {
        drop_analysis();
        solver->init(problem);
        solver->getAssignment().setAssumptions(assumptions);
        lbool res = solver->solve();
        assumptions.clear();
        problem.clear();
        nomodel = (res != l_True);
        return (res == l_Undef) ? 0 :(res == l_True ? 10 : 20);
    }

    int val(int lit) {
        if (nomodel) return 0;
        CandySolverResult& result = solver->getCandySolverResult();
        return result.satisfies(import(lit)) ? lit : -lit;
    }

    int failed(int lit) {
        if (!fmap) analyze();
        int tmp = import(lit).var();
        return fmap[tmp] != 0;
    }

    void set_terminate(void* state, int(*callback)(void* state)) {
        solver->setTermCallback(state, callback);
    }

    void set_learn(void* state, int max_length, void (*callback)(void* state, int* clause)) {
        solver->setLearntCallback(state, max_length, callback);
    }
};

extern "C" {
#include "ipasir.h"

const char* ipasir_signature() {
    return "candy";
}

void* ipasir_init() {
    return new IPASIRCandy();
}

void ipasir_release(void* s) {
    delete (IPASIRCandy*)s;
}

int ipasir_solve(void* s) {
    return ((IPASIRCandy*)s)->solve();
}

void ipasir_add(void* s, int l) {
    ((IPASIRCandy*)s)->add(l);
}

void ipasir_assume(void* s, int l) {
    ((IPASIRCandy*)s)->assume(l);
}

int ipasir_val(void* s, int l) {
    return ((IPASIRCandy*)s)->val(l);
}

int ipasir_failed(void* s, int l) {
    return ((IPASIRCandy*)s)->failed(l);
}

void ipasir_set_terminate(void* s, void* state, int(*callback)(void* state)) {
    ((IPASIRCandy*)s)->set_terminate(state, callback);
}

void ipasir_set_learn(void* s, void* state, int max_length, void (*callback)(void* state, int* clause)) {
    ((IPASIRCandy*)s)->set_learn(state, max_length, callback);
}

};
