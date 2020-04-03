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

#ifndef GATE_ANALYZER
#define GATE_ANALYZER

#include <cstdlib>
#include <algorithm>
#include <memory>

#include <vector>
#include <set>

#include <climits>
#include <chrono>

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"

#include "candy/gates/GateProblem.h"
#include "candy/gates/BlockList.h"

#include "candy/utils/Runtime.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

enum class GateRecognitionMethod {
    Patterns = 0, // use gate patterns
    Semantic = 1, // use semantic checks
    Holistic = 2, // use semantic checks that include the resolution environment
    PatSem = 10, // use gate patterns and semantic checks 
    PatSemHol = 11 // use gate patterns and semantic checks and holistic checks which include the resolution environment
};

class GateAnalyzer {

public:
    GateAnalyzer(const CNFProblem& problem, 
        GateRecognitionMethod method = static_cast<GateRecognitionMethod>(GateRecognitionOptions::method.get()), 
        int tries = GateRecognitionOptions::tries, 
        double timeout = GateRecognitionOptions::timeout);

    ~GateAnalyzer();

    GateProblem& getResult() const {
        return gate_problem;
    }

    GateProblem& getGateProblem() const {
        return gate_problem;
    }

    const CNFProblem& getCNFProblem() const {
        return problem;
    }

    bool hasTimeout() const {
        return runtime.hasTimeout();
    }

    const CNFProblem& problem;
    GateProblem& gate_problem;
    Runtime runtime;

    void analyze();

private:
    // problem to analyze:
    CandySolverInterface* solver;

    // control structures:
    BlockList index; 

    // heuristic configuration:
    unsigned int maxTries = 1;
    bool usePatterns = false;
    bool useSemantic = false;
    bool useHolistic = false;

    // main analysis routines
    void gate_recognition(std::vector<Lit> roots);
    bool isGate(Lit candidate, bool pat, bool sem, bool hol);

    // clause patterns of full encoding
    bool patternCheck(Lit o, const For& fwd, const For& bwd);
    bool semanticCheck(Lit o, const For& fwd, const For& bwd, bool holistic = false);

    bool fixedClauseSize(const For& f, unsigned int n) {
        for (Cl* c : f) if (c->size() != n) return false;
        return true;
    }

};

}
#endif
