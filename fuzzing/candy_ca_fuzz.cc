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

    ClauseDatabase* clause_db1 = new ClauseDatabase();
    Trail* assignment1 = new Trail();
    CandyBuilder<> builder1 { clause_db1, assignment1 };
    CandySolverInterface* solver1 = builder1.build();
    solver1->addClauses(problem);
    solver1->solve();

    ClauseDatabase* clause_db2 = new ClauseDatabase();
    Trail* assignment2 = new Trail();
    CandyBuilder<> builder2 { clause_db2, assignment2 };
    CandySolverInterface* solver2 = builder2.learnThreadSafe().build();
    solver2->addClauses(problem);
    solver2->solve();

    assert(clause_db1->size() == clause_db2->size());

    for (unsigned int i = 0; i < clause_db1->size(); i++) {
        const Clause* clause1 = (*clause_db1)[i];
        const Clause* clause2 = (*clause_db2)[i];
        for (Lit lit : *clause1) {
            assert(clause2->contains(lit));
        }
    }
}