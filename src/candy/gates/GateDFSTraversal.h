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

#ifndef X_EC6785CA_AF27_4D6B_97AB_C774E27E926E_GATEDFSVISITOR_H
#define X_EC6785CA_AF27_4D6B_97AB_C774E27E926E_GATEDFSVISITOR_H

#include <candy/gates/GateAnalyzer.h>

#include <vector>
#include <stack>
#include <unordered_set>

#include <iostream>

namespace Candy {
    
    /**
     * \class GateDFSMarkedGate
     *
     * \ingroup Gates
     *
     * \brief Helper data structure used by visitDFS() for marking gates for
     *   backtracking.
     */
    struct GateDFSMarkedGate {
        const Gate *gate;
        bool backtrackOnNextEncounter;
    };
    
    /**
     * \ingroup Gates
     *
     * DFS-traverses the gate structure given by a Gate analyzer. A Collector object
     * is created, receiving pointers to the gates in order of their traversal
     * rsp. in topological order (backtracking).
     *
     * Collector needs to be a type with the following properties:
     *  - needs to be default-constructible.
     *  - needs to have a method collect(Gate*).
     *  - needs to have a method backtrack(Gate*).
     *  - needs to have a method collectInput(Var).
     *  - needs to have a method init(size_t)
     *  - needs to have a method pruneAt(Gate&) -> bool
     *  - needs to have a method finished()
     * Furthermore, Collector should be move-constructible and move-assignable.
     *
     * Collector.collect() is called whenever a gate is visited the first time,
     * while Collector.backtrack() is called whenever a gate is backtracked during
     * the traversal. Moreover, Collector.collectInput() is called whenever an input
     * variable is encountered the first time.
     * Before performing the DFS, the amount of gates occuring in the gate structure
     * is passed to init(). When the search is completed, finished() is called.
     *
     * A gate g is only visited if pruneAt(g) evaluates to false. 
     *
     *
     * \returns the Collector object having received the gates encountered during
     *   the traversal.
     */
    template<typename Collector>
    Collector traverseDFS(const GateProblem& gates) {
        std::stack<GateDFSMarkedGate> work;
        std::unordered_set<const Gate*> visited;
        Collector collector;
        
        collector.init(gates.nGates());
        visited.reserve(gates.nGates());
        
        // Initialize the work stack
        for (const Cl& clause : gates.getRoots()) {
            for (const Lit lit : clause) {
                const Gate& g = gates[lit.var()];
                if (g.isDefined() && !collector.pruneAt(g)) {
                    work.push(GateDFSMarkedGate{&g, false});
                }
            }
        }
        
        std::unordered_set<Var> seenInputs;
        
        // Visit the gate structure
        while (!work.empty()) {
            GateDFSMarkedGate workItem = work.top();
            work.pop();
            
            if (workItem.backtrackOnNextEncounter) {
                collector.backtrack(workItem.gate);
            }
            else if (visited.find(workItem.gate) == visited.end()) {
                collector.collect(workItem.gate);
                visited.emplace(workItem.gate);
                
                // Put the gate back on the stack, marking it so that next
                // time it is popped, it gets backtracked.
                work.push(GateDFSMarkedGate{workItem.gate, true});
                
                for (auto input : workItem.gate->getInputs()) {
                    const Gate& g = gates[input.var()];
                    if (g.isDefined()
                        && visited.find(&g) == visited.end()
                        && !collector.pruneAt(g)) {
                        work.push(GateDFSMarkedGate{&g, false});
                    }
                    else if (!g.isDefined() && seenInputs.find(input.var()) == seenInputs.end()) {
                        seenInputs.emplace(input.var());
                        collector.collectInput(input.var());
                    }
                }
            }
        }
        
        collector.finished();
        return collector;
    }
    
    /*
     * \ingroup Gates
     *
     * TODO: documentation
     */
    class TopologicallyOrderedGates {
    public:
        TopologicallyOrderedGates()
        : m_backtrackOutputOrder() {
        }
        
        void backtrack(const Gate* g) {
            m_backtrackOutputOrder.push_back(g->getOutput().var());
        }
        
        void collect(const Gate* g) {
            (void)g;
        }
        
        void collectInput(Var var) {
            (void)var;
        }
        
        void init(size_t n) {
            m_backtrackOutputOrder.reserve(n);
        }
        
        const std::vector<Var>& getOutputsOrdered() {
            return m_backtrackOutputOrder;
        }
        
        bool pruneAt(const Gate& g) {
            (void)g;
            return false;
        }
        
        void finished() {
        }
        
    private:
        std::vector<Var> m_backtrackOutputOrder;
    };

    /*
     * \ingroup Gates
     *
     * TODO: documentation
     */
    inline TopologicallyOrderedGates getTopoOrder(const GateProblem& gates) {
        return traverseDFS<TopologicallyOrderedGates>(gates);
    }
}

#endif
