
#include "candy/frontend/CandyCommandLineParser.h"

using namespace Candy;

int main(int argc, char** argv) {
    GlucoseArguments args = parseCommandLineArgs(argc, argv);

    CNFProblem problem{};
    try {
        if (args.read_from_stdin) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            problem.readDimacsFromStdin();
            return 0;
        } else {
            problem.readDimacsFromFile(args.input_filename);
            return 0;
        }
    } 
    catch (ParserException& e) {
		printf("Caught Parser Exception\n%s\n", e.what());
        return 0;
    }

    return 1;
}