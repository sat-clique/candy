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

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <cassert>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <algorithm>

#include "candy/core/CNFProblem.h"
#include "candy/core/SolverTypes.h"
#include "candy/gates/GateAnalyzer.h"
#include "candy/gates/AIGProblem.h"

using namespace Candy;

int main(int argc, char** argv) {
	if (argc == 1) {
		fprintf(stderr, "Usage: cnf2aig [parameters] file\n");
		exit(1);
	}

	FILE* out = stdout;
    char* filename = argv[1];

	CNFProblem problem;
    problem.readDimacsFromFile(filename);

	bool trivially_unsat = false;
	for (Cl* clause : problem) {
		trivially_unsat |= clause->size() == 0;
	}

    if (problem.nClauses() == 0) { // trivial SAT
		fprintf(out, "aag 0 0 0 1 0\n1\n");
	}
    else if (problem.nVars() == 0 || trivially_unsat) { // trivial UNSAT
		fprintf(out, "aag 0 0 0 1 0\n0\n");
	}
	else {
		GateAnalyzer gates { problem };
		gates.analyze();
		gates.getGateProblem().normalizeRoots();

		AIGProblem aig(gates.getGateProblem());
		aig.createAIG();
		aig.printAIG(out);
	}
}


