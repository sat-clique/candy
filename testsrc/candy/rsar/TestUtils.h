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

#ifndef X_3D4F7FAE_8852_4861_A031_1808523E1215_TESTUTILS_H
#define X_3D4F7FAE_8852_4861_A031_1808523E1215_TESTUTILS_H

#include <core/SolverTypes.h>
#include <simp/SimpSolver.h>
#include <rsar/Heuristics.h>
#include <randomsimulation/Conjectures.h>

#include "SolverMock.h"

#include <vector>
#include <memory>
#include <queue>


namespace Candy {
    class EquivalencyChecker {
    public:
        EquivalencyChecker();
        void addClauses(const std::vector<Cl>& clauses);
        Var createVariable();
        
        void createVariables(Var max);
        
        void finishedAddingRegularVariables();
        
        bool isEquivalent(const std::vector<Lit>& assumptions, Lit a, Lit b);
        bool isAllEquivalent(const std::vector<Lit>& assumptions, const std::vector<Lit>& equivalentLits);
        bool isBackbones(const std::vector<Lit>& assumptions, const std::vector<Lit>& backboneLits);
        
    private:
        bool solve(const std::vector<Lit>& assumptions);
        
        std::unique_ptr<Glucose::SimpSolver> m_solver;
        Var m_maxVar;
    };
    
    
    /** 
     * Returns true iff first and second occur in events, and second does
     * not occur before first.
     */
    bool occursOnlyBefore(const std::vector<SolverMockEvent> &events,
                          SolverMockEvent first,
                          SolverMockEvent second);
    
    /**
     * Returns true iff first and second occur in events, and first
     * does not occur before second.
     */
    bool occursOnlyAfter(const std::vector<SolverMockEvent> &events,
                         SolverMockEvent first,
                         SolverMockEvent second);
    
    /**
     * Returns true iff first and second occur in events, and first
     * occurs before second.
     */
    bool occursBefore(const std::vector<SolverMockEvent> &events,
                      SolverMockEvent first,
                      SolverMockEvent second);
    
    /**
     * Returns true iff first and second occur in events, and first
     * occurs before after.
     */
    bool occursAfter(const std::vector<SolverMockEvent> &events,
                     SolverMockEvent first,
                     SolverMockEvent second);
    
    /** Prints the given solver mock event log. */
    void printEventLog(const std::vector<SolverMockEvent> &events, bool insertNewlines = false);
    
    /** Returns true iff the given clause contains the given literal. */
    bool contains(const Cl& clause, Lit lit);
    
    /** Creates an EquivalenceConjecture containing the given literals. */
    EquivalenceConjecture createEquivalenceConjecture(const std::vector<Lit> &lits);
    
    /** Returns true iff the given variable contains in any of the given clauses. */
    bool varOccursIn(const std::vector<Cl> &clauses, Var var);
    
    /** Randomly creates n distinct literals with variables in the range of 0 and
     * the maximum variable occuring in the given problem instance f. */
    std::vector<Lit> pickLiterals(const CNFProblem &f, int n);
    
    /** Returns 10 random indices of the given vector of literals, using the given
     * RNG seed. */
    std::priority_queue<std::vector<Lit>::size_type> getRandomIndices(const std::vector<Lit> &literals,
                                                                      unsigned int seed);
    
    /** Returns a partitioning of the given vector of literals corresponding to the given
     * indices. Clears its arguments. */
    std::vector<std::vector<Lit>> convertToPartition(std::vector<Lit> &literals,
                                                     std::priority_queue<std::vector<Lit>::size_type> &indices);
    
    /** Creates a (determinized) pseudorandom Conjectures object for the given literals. */
    std::unique_ptr<Conjectures> createRandomConjectures(const std::vector<Lit> &literals,
                                                         const CNFProblem& problem);
    
    /** Creates a (determinized) pseudorandomly behaving deactivation heuristic for
     * the given literals, eventually marking all literals for deactivation. */
    std::unique_ptr<RefinementHeuristic> createRandomHeuristic(const std::vector<Lit> &literals);
}

#endif
