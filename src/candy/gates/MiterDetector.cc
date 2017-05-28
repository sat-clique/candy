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

#include "MiterDetector.h"
#include <candy/gates/GateAnalyzer.h>
#include <candy/gates/GateDFSTraversal.h>
#include <candy/core/SolverTypes.h>

#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace Candy {
    namespace {
        
        /**
         * \ingroup Gates
         *
         * \brief collects the monotonously nested "border" gates at the edge of the
         *   monotonously nested region of the gate structure.
         */
        class MonotonousRegionBorderGateCollector {
        public:
            
            void backtrack(const Gate* g) {
                bool isBorderGate = true;
                for (Lit input : g->getInputs()) {
                    Var inputVar = var(input);
                    isBorderGate &= m_seenOuts.find(inputVar) == m_seenOuts.end();
                }
                
                if (isBorderGate) {
                    m_monotonousRegionBorder.insert(g);
                }
            }
            
            void collect(const Gate* g) {
                m_seenOuts.insert(var(g->getOutput()));
            }
            
            void collectInput(Var var) const {
                (void) var;
            }
            
            void init(size_t n) const {
                (void) n;
            }
            
            bool pruneAt(const Gate& g) {
                // The nonmonotonously nested part of the gate structure is not taken
                // into account by this heuristic
                m_seenNonmonotonous |= g.hasNonMonotonousParent();
                return g.hasNonMonotonousParent();
            }
            
            void finished() {
                m_seenOuts.clear();
            }
            
            const std::unordered_set<const Gate*>& getMonotonousRegionBorder() const {
                return m_monotonousRegionBorder;
            }
            
            bool hasSeenNonmonotonouslyNestedGate() {
                return m_seenNonmonotonous;
            }
            
        private:
            std::unordered_set<Var> m_seenOuts;
            std::unordered_set<const Gate*> m_monotonousRegionBorder;
            bool m_seenNonmonotonous = false;
        };
        
        /**
         * \ingroup Gates
         *
         * \brief checks if all given gates have exactly two input variables
         *
         * \returns true iff G only contains gates with exactly two input variables.
         */
        template<typename CollectionType>
        bool allGatesHaveTwoInputs(CollectionType& gates) {
            std::unordered_set<Var> vars;
            
            for(const Gate* g : gates) {
                vars.clear();
                for (auto input : g->getInputs()) {
                    vars.insert(var(input));
                }
                if (vars.size() != 2) {
                    return false;
                }
            }
            return true;
        }
        
        class SignOccurence {
        public:
            void setOccured(bool sign) {
                if (sign) {
                    m_positive = true;
                }
                else {
                    m_negative = true;
                }
            }
            
            bool hasOccured(bool sign) {
                return sign ? m_positive : m_negative;
            }
            
            bool isSaturated() {
                return m_positive && m_negative;
            }
            
        private:
            bool m_positive = false;
            bool m_negative = false;
        };
        
        /**
         * \ingroup Gates
         *
         * \brief checks whether all given gates gates are potentially the two
         *    input-receiving gates of an AIG-encoded binary XOR.
         *
         * \param gates     A collection G of gates, each with exactly 2 input variables.
         *
         * \returns         true iff G is a collection of gates as specified.
         */
        template<typename CollectionType>
        bool allGatesAreAIGXORsWithUniqueInputVars(CollectionType& gates) {
            std::unordered_map<Var, SignOccurence> literalOccurences;
            
            for (const Gate* gate : gates) {
                for (Lit input : gate->getInputs()) {
                    Var inpVar = var(input);
                    bool inpSign = sign(input);
                    
                    if (literalOccurences[inpVar].hasOccured(inpSign)) {
                        return false;
                    }
                    literalOccurences[inpVar].setOccured(inpSign);
                }
            }
            
            for (auto kv : literalOccurences) {
                if (!kv.second.isSaturated()) {
                    return false;
                }
            }
            
            return true;
        }
    
        /**
         * \ingroup Gates
         *
         * \brief checks whether the given gate is a binary XOR gate.
         *
         * \param g     An arbitrary gate.
         *
         * \returns     true iff g is a binary XOR gate.
         */
        bool isBinaryXORGate(const Gate& g) {
            auto& clauses = g.getForwardClauses();
            if (clauses.size() != 2ull) {
                return false;
            }
            auto& clause1 = *(clauses[0]);
            auto& clause2 = *(clauses[1]);
            
            if (clause1.size() != 3ull || clause2.size() != 3ull) {
                return false;
            }
            
            // TODO: optimize allocations by reusing gateLits
            std::unordered_set<Lit> gateLits;
            gateLits.insert(clause1.begin(), clause1.end());
            gateLits.insert(clause2.begin(), clause2.end());
            return gateLits.size() == 5ull;
        }
        
        /**
         * \ingroup Gates
         *
         * \brief checks whether the given gates are binary XOR gates with
         *   unique input variables (wrt. the other given gates)
         *
         * \param g     A collection of gates G.
         *
         * \returns true    iff G is a collection of binary XOR gates as described.
         */
        template<typename CollectionType>
        bool allGatesAreBinaryXORsWithUniqueInputVars(CollectionType& gates) {
            std::unordered_set<Var> seenInputVars;
            
            for (const Gate* gate : gates) {
                if (!isBinaryXORGate(*gate)) {
                    return false;
                }
                
                for(Lit input : gate->getInputs()) {
                    if (seenInputVars.find(var(input)) != seenInputVars.end()) {
                        return false;
                    }
                }
                
                for(Lit input : gate->getInputs()) {
                    seenInputVars.insert(var(input));
                }
            }
            
            return true;
        }
    }
    
    bool hasPossiblyMiterStructure(const GateAnalyzer& analyzer) noexcept {
        auto borderGateCollector = traverseDFS<MonotonousRegionBorderGateCollector>(analyzer);
        auto& borderGates = borderGateCollector.getMonotonousRegionBorder();
        
        // Some quick preliminary checks to quickly rule out non-miters
        if (borderGates.empty()
            || !borderGateCollector.hasSeenNonmonotonouslyNestedGate()
            || !allGatesHaveTwoInputs(borderGates)) {
            return false;
        }
        
        return  (allGatesAreAIGXORsWithUniqueInputVars(borderGates)
                 || allGatesAreBinaryXORsWithUniqueInputVars(borderGates));
    }
}
