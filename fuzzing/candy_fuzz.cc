#include "minisat/core/Solver.h"

#include "candy/core/Solver.h"
#include "candy/frontend/CandyCommandLineParser.h"
#include "candy/frontend/SolverFactory.h"
#include "candy/core/branching/VSIDS.h"
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