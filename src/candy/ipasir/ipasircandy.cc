/* Copyright(C) 2017, Markus Iser, Adapted IPASIR Interface for Candy Solver
 * (freely used parts of Armin Biere's adaption of Minisat from the examples in the IPASIR package) */

#include "candy/core/Solver.h"
#include "candy/core/SolverTypes.h"

#include <vector>

using namespace std;
using namespace Candy;
using namespace Glucose;

class IPASIRCandy {

  Solver solver;

  vector<Lit> assumptions;
  vector<Lit> clause;

  unsigned char* fmap;
  bool nomodel;

  Lit import(int lit) {
    while((size_t)abs(lit) > solver.nVars()) (void)solver.newVar();
    return mkLit(Var(abs(lit)-1), (lit < 0));
  }

  void analyze() {
    fmap = new unsigned char [solver.nVars()];
    memset(fmap, 0, solver.nVars());
    for (unsigned int i = 0; i < solver.getConflict().size(); i++) {
      fmap[var(solver.getConflict()[i])] = 1;
    }
  }

  void drop_analysis() {
    if(fmap) delete [] fmap;
    fmap = 0;
  }

public:
  IPASIRCandy() : fmap(0), nomodel(false) {
    solver.verbosity = 0;
  }

  ~IPASIRCandy() {
    drop_analysis();
  }

  void add(int lit) {
    drop_analysis();
    nomodel = true;
    if (lit) {
      clause.push_back(import(lit));
    } else {
      solver.addClause(clause);
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
    lbool res = solver.solveLimited(assumptions);
    assumptions.clear();
    nomodel = (res != l_True);
    return (res == l_Undef) ? 0 :(res == l_True ? 10 : 20);
  }

  int val(int lit) {
    if (nomodel) return 0;
    lbool res = solver.modelValue(import(lit));
    return (res == l_True) ? lit : -lit;
  }

  int failed(int lit) {
    if (!fmap) analyze();
    int tmp = var(import(lit));
    return fmap[tmp] != 0;
  }

  void set_terminate(void* state, int(*callback)(void* state)) {
    solver.setTermCallback(state, callback);
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

};
