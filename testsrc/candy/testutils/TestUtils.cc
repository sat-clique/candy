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
        Cl clause1(clause.begin(), clause.end());
        std::sort(clause1.begin(), clause1.end());
        for (auto fcl : formula) {
            Cl clause2(fcl->begin(), fcl->end());
            std::sort(clause2.begin(), clause2.end());
            found |= (clause1 == clause2);
        }
        return found;
    }
    
    double getMaxAbsDifference(const std::unordered_map<std::uint8_t, double>& sample1,
                               const std::unordered_map<std::uint8_t, double>& sample2) {
        double diff = 0.0f;
        
        assert (sample1.size() == sample2.size());
        for (auto kv : sample1) {
            std::uint8_t i = kv.first;
            double localDiff = std::abs(sample1.at(i) - sample2.at(i));
            diff = std::max(diff, localDiff);
        }
        return diff;
    }
}
