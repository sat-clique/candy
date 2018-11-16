#include "minisat/core/Solver.h"

#include "candy/core/Solver.h"
#include "candy/frontend/CandyCommandLineParser.h"
#include "candy/frontend/SolverFactory.h"
#include "candy/core/branching/VSIDS.h"
#include "candy/frontend/CandyBuilder.h"
#include "util.h"

using namespace Candy;

int main(int argc, char** argv) {
    GlucoseArguments args = parseCommandLineArgs(argc, argv);

    CNFProblem problem{};
    try {
        if (args.read_from_stdin) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            problem.readDimacsFromStdin();
        } else {
            problem.readDimacsFromFile(args.input_filename);
        }
    } 
    catch (ParserException& e) {
		printf("Caught Parser Exception\n%s\n", e.what());
        return 0;
    }

    if (problem.nVars() > 100) {
        return 0; // concentrate on small problems during fuzzing
    }

    CandySolverInterface* solver;

    ClauseDatabase* clause_db = new ClauseDatabase();
    Trail* assignment = new Trail();

    CandyBuilder<> builder { clause_db, assignment };

    if (SolverOptions::opt_use_lrb) {
        if (SolverOptions::opt_use_ts_ca) {
            if (SolverOptions::opt_use_ts_pr) {
                solver = builder.branchWithLRB().learnThreadSafe().propagateThreadSafe().build();
            } else {
                solver = builder.branchWithLRB().learnThreadSafe().build();
            }
        } else {
            if (SolverOptions::opt_use_ts_pr) {
                solver = builder.branchWithLRB().propagateThreadSafe().build();
            } else {
                solver = builder.branchWithLRB().build();
            }
        }
    } else {
        if (SolverOptions::opt_use_ts_ca) {
            if (SolverOptions::opt_use_ts_pr) {
                solver = builder.learnThreadSafe().propagateThreadSafe().build();
            } else {
                solver = builder.learnThreadSafe().build();
            }
        } else {
            if (SolverOptions::opt_use_ts_pr) {
                solver = builder.propagateThreadSafe().build();
            } else {
                solver = builder.build();
            }
        }
    } 

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

    if (result == minisat_result(problem)) {
        return 0;
    } 
    else {
        return 1;
    }
}