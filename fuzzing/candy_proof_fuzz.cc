#include "minisat/core/Solver.h"
#include "drat-trim.h"

#include "candy/frontend/Exceptions.h"
#include "candy/frontend/SolverFactory.h"
#include "candy/systems/branching/VSIDS.h"
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

    SolverOptions::opt_certified_file = "proof.drat";

    CandySolverInterface* solver = createSolver();
    solver->init(problem);

    lbool result = solver->solve();

    lbool reference_result = minisat_result(problem);
    assert (result == reference_result);

    if (result == l_False) {
        int proof_result = check_proof((char*)inputFilename, SolverOptions::opt_certified_file.get()); 
        assert (0 == proof_result);
    }
}