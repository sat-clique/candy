/* Copyright (c) 2017 Felix Kutzner
 
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

#ifndef _3EC37B3C_8FFE_4765_8E74_F1177D8BF870_TESTUTILS_H
#define _3EC37B3C_8FFE_4765_8E74_F1177D8BF870_TESTUTILS_H

#include <core/SolverTypes.h>
#include <unordered_set>
#include <vector>


namespace Glucose {
    class CNFProblem;
}


namespace randsim {
    class Conjectures;
    class EquivalenceConjecture;
    
    void deleteClauses(Glucose::CNFProblem* formula);
    void assertContainsVariable(const std::unordered_set<Glucose::Var>& variables, const Glucose::Var forbidden);
    void assertDoesNotContainVariable(const std::unordered_set<Glucose::Var>& variables, const Glucose::Var forbidden);
    Glucose::Cl negatedLits(const Glucose::Cl& clause);
    void insertVariables(const std::vector<Glucose::Lit>& lits, std::unordered_set<Glucose::Var>& target);
    
    
    bool hasBackboneConj(Conjectures &c, Glucose::Lit lit);
    bool isEquivalenceConjEq(EquivalenceConjecture &conj, std::vector<Glucose::Lit> lits);
    bool hasEquivalenceConj(Conjectures &c, std::vector<Glucose::Lit> lits);
    
    bool containsClause(const Glucose::For& formula, const Glucose::Cl& clause);
}

#endif
