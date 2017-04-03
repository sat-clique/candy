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

#include "TestUtils.h"
#include <utils/CNFProblem.h>

namespace Candy {
    void assertContainsVariable(const std::unordered_set<Var>& variables, Var forbidden) {
        assert(variables.find(forbidden) != variables.end()); // TODO refactor
    }
    
    void assertDoesNotContainVariable(const std::unordered_set<Var>& variables, Var forbidden) {
        assert(variables.find(forbidden) == variables.end()); // TODO refactor
    }
    
    void deleteClauses(CNFProblem* formula) {
        
        // TODO: This is a weird workaround. CNFProblem ought to own the formula and take care of its destruction.
        
        if (formula == nullptr) {
            return;
        }
        
        for (auto clause : formula->getProblem()) {
            delete clause;
        }
        formula->getProblem().clear();
    }
    
    Cl negatedLits(const Cl& clause) {
        Cl result;
        for (auto lit : clause) {
            result.push_back(~lit);
        }
        return result;
    }
    
    void insertVariables(const std::vector<Lit>& lits, std::unordered_set<Var>& target) {
        for (auto lit : lits) {
            target.insert(var(lit));
        }
    }

    bool containsClause(const For& formula, const Cl& clause) {
        bool found = false;
        for (auto fcl : formula) {
            found |= (*fcl == clause); // TODO: make this independent of literal positions
        }
        return found;
    }
}
