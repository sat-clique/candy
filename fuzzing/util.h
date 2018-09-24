#ifndef _FUZZING_UTIL_H
#define _FUZZING_UTIL_H

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"

extern Candy::lbool minisat_result(Candy::CNFProblem const &problem);

#endif