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

#ifndef X_15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H
#define X_15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H

#include <vector>
#include <memory>
#include <unordered_set>

#include <core/SolverTypes.h>

namespace Candy {
  class GateAnalyzer;
}

namespace randsim {
    
    /**
     * \class GateFilter
     *
     * \ingroup RandomSimulation
     *
     * \brief A restriction of gates.
     *
     * GateFilter objects are responsible for determining sets of enabled gates,
     * identified by their output variable.
     *
     * Usage example: build a RandomSimulation object with this strategy.
     */
    class GateFilter {
    public:
        /**
         * Retrieves the set of gates marked enabled by the filter, represented
         * as a collection of their output variables.
         */
        virtual std::unordered_set<Glucose::Var> getEnabledOutputVars() = 0;
        
        virtual ~GateFilter();
        GateFilter(const GateFilter& other) = delete;
        GateFilter& operator=(const GateFilter& other) = delete;
    };
    
    /**
     * \class ClauseOrder
     *
     * \ingroup RandomSimulation
     *
     * \brief A topologically sorted representation of a gate structure.
     *
     * ClauseOrder objects are responsible for representing a gate structure in a
     * topologically ordered way - with variables considered as nodes, and a->b being
     * an edge iff b is the output variable of a gate having a as an input variable.
     *
     * Usage example: build a RandomSimulation object with this strategy.
     */
    class ClauseOrder {
    public:
        /**
         * Establishes the order using the gate structure contained in the given gate analyzer.
         */
        virtual void readGates(Candy::GateAnalyzer& analyzer) = 0;
        
        /**
         * Retrieves a collection of the variables which are not gate-output variables.
         */
        virtual const std::vector<Glucose::Var> &getInputVariables() const = 0;
        
        /**
         * Retrieves a topologically-ordered sequence of gate-output literals.
         * If a gate filter is used, the following output literals are omitted:
         *   - literals O whose variable is not marked enabled by the gate filter
         *   - literals O' which are outputs of a gate whose value (transitively)
         *     depends on a variable not marked enabled by the gate filter.
         */
        virtual const std::vector<Glucose::Lit> &getGateOutputsOrdered() const = 0;
        
        /**
         * For a variable V occuring in getGateOutputsOrdered() as the literal L,
         * get all clauses containing L encoding the rsp. gate. Note: the result is
         * not empty, and contains either the forward or the backward clauses
         * of the gate. If the gate is nested monotonically, these clauses are the
         * gate's forward clauses.
         */
        virtual const std::vector<const Candy::Cl*> &getClauses(Glucose::Var variable) const = 0;
        
        /**
         * Sets a gate filter defining the output variables to be regarded
         * (see getGateOutputsOrdered()). If no such filter is set, all variables
         * are considered to be enabled.
         */
        virtual void setGateFilter(std::unique_ptr<GateFilter> gateFilter) = 0;

        /**
         * Retrieves the max. variable index found in the gate structure, plus one.
         * TODO: rename, improve documentation
         */
        virtual unsigned int getAmountOfVars() const = 0;
        
        ClauseOrder();
        virtual ~ClauseOrder();
        ClauseOrder(const ClauseOrder& other) = delete;
        ClauseOrder& operator=(const ClauseOrder& other) = delete;
    };
    
    /**
     * Creates an object of the simple (recursive) implementation of ClauseOrder.
     */
    std::unique_ptr<ClauseOrder> createRecursiveClauseOrder();
}

#endif
