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

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/systems/propagate/Propagate.h"

#ifndef CANDY_DRAT_CHECKER
#define CANDY_DRAT_CHECKER

typedef struct gzFile_s *gzFile;

namespace Candy {

class DRATChecker {

private:
    CNFProblem& problem;

    ClauseDatabase clause_db;
    Trail trail;
    Propagate propagator;

    std::vector<std::vector<Clause*>> occurences;

    unsigned int num_deleted;

public:
    DRATChecker(class CNFProblem& problem_);
    ~DRATChecker();

    bool check_proof(const char* filename);
    long proof_size(const char* filename);

private:
    bool check_proof(gzFile input_stream);

    template <typename Iterator>
    bool check_clause_add(Iterator begin, Iterator end);

    template <typename Iterator>
    bool check_asymm(Iterator begin, Iterator end, Lit lit);

    template <typename Iterator>
    bool check_clause_remove(Iterator begin, Iterator end);

    void cleanup_deleted();

};

}

#endif
