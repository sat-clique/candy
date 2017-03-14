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

#include "Heuristics.h"

#include <rsar/Refinement.h>
#include <randomsimulation/GateDFSTraversal.h>
#include <utils/MemUtils.h>

#include <unordered_map>

namespace Candy {
    RefinementHeuristic::RefinementHeuristic() {
        
    }
    
    RefinementHeuristic::~RefinementHeuristic() {
        
    }
    
    namespace {
        /**
         * Adds the given variables to the target's variable removal work queue.
         *
         * S must be a type having the method addVariableRemovalToWorkQueue(Var)
         * and must be iterable using a range-based for loop, yielding elements
         * of type T. The function transform of signature Var(T) is used to
         * transform items of type T to variables getting added to the variable
         * removal work queue.
         */
        template<typename S, typename T>
        void genericMarkRemovals(S& target,
                                 std::function<Var(T)> transform,
                                 std::unordered_set<Var>& toBeDeactivated) {
            if (toBeDeactivated.empty()) {
                return;
            }
            
            for (auto untransformed : target) {
                Var v = transform(untransformed);
                if (toBeDeactivated.find(v) != toBeDeactivated.end()) {
                    target.addVariableRemovalToWorkQueue(v);
                    toBeDeactivated.erase(v);
                }
            }
        }
        
        /**
         * Given a gate analyzer and a vector gateOutputs of gate output variables, getDirectlyNestedGates
         * computes a mapping of gates G to the gates G' for which G is directly nested in G'. The mapping's
         * domain consists of all gates having an output variable in gateOutputs.
         */
        std::unordered_map<Gate*, std::vector<Gate*>> getDirectlyNestedGates(GateAnalyzer &analyzer,
                                                                             const std::vector<Var> gateOutputs) {
            std::unordered_map<Gate*, std::vector<Gate*>> result;
            
            for (auto output : gateOutputs) {
                auto& dependentGate = analyzer.getGate(mkLit(output, 1));
                for (auto inputLit : dependentGate.getInputs()) {
                    auto &gate = analyzer.getGate(inputLit);
                    if (gate.isDefined()) {
                        result[&gate].push_back(&dependentGate);
                    }
                }
            }
            
            return result;
        }
        
        /**
         * Given a gate analyzer, countInputDependencies computes for each gate output variable v
         * the amount of inputs on which the value of v depends.
         */
        std::unordered_map<Var, size_t> countInputDependencies(GateAnalyzer& analyzer) {
            // for each gate, get the set of gates which depend on the output
            auto topo = getTopoOrder(analyzer);
            auto dependents = getDirectlyNestedGates(analyzer, topo.getOutputsOrdered());
            
            // in topological order, do for each gate g (with output o)
            //   - compute the set of input variables
            //   - store the size of the input variable set
            //   - distribute the set of input variables to all gates in which g is directly nested
            //   - clear the set of input variables for g to save memory (due to the ordering,
            //     it is not necessary to visit g again)
            // i.e. the input dependency sets are "pushed" through the gate structure.
            std::unordered_map<Gate*, std::unordered_set<Var>> inputDependencies;
            std::unordered_map<Var, size_t> inputDependencyCount;
            
            for (auto gateOutput : topo.getOutputsOrdered()) {
                auto& gate = analyzer.getGate(mkLit(gateOutput));
                auto& inputsViaDependencies = inputDependencies[&gate];
                
                for (auto inpLit : gate.getInputs()) {
                    if (!analyzer.getGate(inpLit).isDefined()) {
                        auto inpVar = var(inpLit);
                        inputsViaDependencies.insert(inpVar);
                    }
                }
                inputDependencyCount[var(gate.getOutput())] = inputsViaDependencies.size();
                
                // move the information about input variables to the gates depending on this one
                for (auto dependent : dependents[&gate]) {
                    inputDependencies[dependent].insert(inputsViaDependencies.begin(),
                                                        inputsViaDependencies.end());
                }
                inputDependencies.erase(&gate);
            }
            
            return inputDependencyCount;
        }
        
        
        class InputDepCountRefinementHeuristic : public RefinementHeuristic {
        public:
            void beginRefinementStep() override;
            void markRemovals(EquivalenceImplications& equivalence) override;
            void markRemovals(Backbones& backbones) override;
            
            InputDepCountRefinementHeuristic(GateAnalyzer& analyzer, const std::vector<size_t>& config);
            virtual ~InputDepCountRefinementHeuristic();
            InputDepCountRefinementHeuristic(const InputDepCountRefinementHeuristic &other) = delete;
            InputDepCountRefinementHeuristic& operator= (const InputDepCountRefinementHeuristic& other) = delete;
            
        private:
            void init();
            std::vector<std::unordered_set<Var>> m_deactivationsByStep;
            std::vector<size_t> m_config;
            GateAnalyzer& m_analyzer;
            int m_step;
        };
        
        InputDepCountRefinementHeuristic::InputDepCountRefinementHeuristic(GateAnalyzer& analyzer,
                                                                           const std::vector<size_t>& config)
        : RefinementHeuristic(),
        m_deactivationsByStep(),
        m_config(config),
        m_analyzer(analyzer),
        m_step(0) {
        }
        
        InputDepCountRefinementHeuristic::~InputDepCountRefinementHeuristic() {
        }

        void InputDepCountRefinementHeuristic::init() {
            auto inputSizes = countInputDependencies(m_analyzer);
            
            // Note: this code is written under the assumption that m_config
            // is a very small vector (about 4 or 5 elements) - for large
            // vectors, this should possibly be rewritten using a predecessor
            // search.
            m_deactivationsByStep.resize(m_config.size()+1);
            for(auto outputVarAndSize : inputSizes) {
                Var outputVar = outputVarAndSize.first;
                size_t size = outputVarAndSize.second;
                
                size_t i = 0;
                for (; i < m_config.size(); ++i) {
                    if (size > m_config[i]) {
                        m_deactivationsByStep[i].insert(outputVar);
                        break;
                    }
                }
                
                if (i == m_config.size()) {
                    m_deactivationsByStep.back().insert(outputVar);
                }
            }
        }
        
        void InputDepCountRefinementHeuristic::beginRefinementStep() {
            if (m_step == 0) {
                init();
            }
            ++m_step;
        }
        
        void InputDepCountRefinementHeuristic::markRemovals(EquivalenceImplications& equivalence) {
            assert(m_step > 0);
            genericMarkRemovals<EquivalenceImplications, Implication>(equivalence,
                                                                      [](Implication litPair) {
                                                                          return var(litPair.first);
                                                                      },
                                                                      m_deactivationsByStep[m_step-1]);
        }
        
        void InputDepCountRefinementHeuristic::markRemovals(Backbones& backbones) {
            assert(m_step > 0);
            genericMarkRemovals<Backbones, Lit> (backbones,
                                                 [](Lit lit) {
                                                     return var(lit);
                                                 },
                                                 m_deactivationsByStep[m_step-1]);
        }
    }
    
    std::unique_ptr<RefinementHeuristic> createInputDepCountRefinementHeuristic(GateAnalyzer& analyzer,
                                                                                const std::vector<size_t>& config) {
        return backported_std::make_unique<InputDepCountRefinementHeuristic>(analyzer,
                                                                             config);
    }
    
}
