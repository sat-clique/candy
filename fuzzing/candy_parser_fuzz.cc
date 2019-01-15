#include "candy/core/Solver.h"
#include "candy/frontend/Exceptions.h"
#include "candy/frontend/CandyBuilder.h"
#include "util.h"

using namespace Candy;

int main(int argc, char** argv) {
    parseOptions(argc, argv, true);

    CNFProblem problem{};
    try {
        if (argc == 1) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            problem.readDimacsFromStdin();
        } else {
            const char* inputFilename = argv[1];
            problem.readDimacsFromFile(inputFilename);
        }
    }
    catch (ParserException& e) {
		printf("Caught Parser Exception\n%s\n", e.what());
        return 0;
    }

    return 1;
}