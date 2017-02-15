/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#ifndef X_4311DBC6_D73E_4BCA_A592_823CDA996EB9_TESTUTILS_H
#define X_4311DBC6_D73E_4BCA_A592_823CDA996EB9_TESTUTILS_H

#include <core/SolverTypes.h>
#include <unordered_set>
#include <vector>


namespace Candy {
    class CNFProblem;
}

namespace Candy {
    class Conjectures;
    class EquivalenceConjecture;
    
    /** \defgroup TestUtils */
    
    /**
     * \ingroup TestUtils
     *
     * Deletes the clauses in the given formula.
     */
    void deleteClauses(CNFProblem* formula);
    
    /**
     * \ingroup TestUtils
     *
     * Asserts that variables does not contain the variable forbidden.
     */
    void assertContainsVariable(const std::unordered_set<Glucose::Var>& variables, const Glucose::Var forbidden);
    
    /**
     * \ingroup TestUtils
     *
     * Asserts that variables contains the variable required.
     */
    void assertDoesNotContainVariable(const std::unordered_set<Glucose::Var>& variables, const Glucose::Var required);
    
    /**
     * \ingroup TestUtils
     *
     * Returns a clause containing the negated lits of the given clause.
     */
    Cl negatedLits(const Cl& clause);
    
    /**
     * \ingroup TestUtils
     *
     * Inserts the variables contained in the literals of lits in to the target set.
     */
    void insertVariables(const std::vector<Glucose::Lit>& lits, std::unordered_set<Glucose::Var>& target);
    
    /**
     * \ingroup TestUtils
     *
     * Returns true iff the given formula contains the given clause.
     */
    bool containsClause(const For& formula, const Cl& clause);
}

#endif
