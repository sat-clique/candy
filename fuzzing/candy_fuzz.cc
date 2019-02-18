#include "minisat/core/Solver.h"

#include "candy/frontend/Exceptions.h"
#include "candy/frontend/CandyBuilder.h"
#include "util.h"

using namespace Candy;

int main(int argc, char** argv) {
    parseOptions(argc, argv, true);

    CNFProblem problem{};
    const char* inputFilename;
    try {
        if (argc == 1) {
            return 0;
        } else {
            inputFilename = argv[1];
            problem.readDimacsFromFile(inputFilename);
        }
    }
    catch (ParserException& e) {
		printf("c Caught Parser Exception\n%s\n", e.what());
        return 0;
    }

    if (problem.nVars() > 100) {
        return 0; // concentrate on small problems during fuzzing
    }

    CandySolverInterface* solver = createSolver();
    solver->init(problem);

    lbool result = solver->solve();

    if (result == minisat_result(problem)) {
        return 0;
    } 
    else {
        return 1;
    }
}