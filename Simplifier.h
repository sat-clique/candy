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

#ifndef SRC_CANDY_SIMPLIFIER_H_

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"

template<class TPropagate> 
class Simplifier {

    ClauseDatabase& clause_db;
    Trail& trail;

    SubsumptionClauseDatabase database;

    Subsumption subsumption;
    VariableElimination elimination;

    bool conflict;

    Simplifier(ClauseDatabase& clause_db_, Trail& trail_) 
     : clause_db(clause_db_), trail(trail_), 
        database(clause_db, trail), 
        subumption(database), elimination(clause_db, trail), conflict(false) {
    }

public:

    bool hasConflict() {
        return conflict;
    }

    

}

#endif