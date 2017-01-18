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

#include "ClauseOrder.h"

#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <gates/GateAnalyzer.h>
#include <core/SolverTypes.h>

namespace randsim {
    ClauseOrder::ClauseOrder() {
        
    }
    
    ClauseOrder::~ClauseOrder() {
        
    }
    
    class RecursiveClauseOrder : public ClauseOrder {
    public:
        void readGates(GateAnalyzer& analyzer) override;
        const std::vector<Glucose::Var> &getInputVariables() const override;
        const std::vector<Glucose::Lit> &getGateOutputsOrdered() const override;
        const std::vector<const Glucose::Cl*> &getClauses(Glucose::Var variable) const override;
        unsigned int getAmountOfVars() const override;
        
        RecursiveClauseOrder();
        virtual ~RecursiveClauseOrder();
        RecursiveClauseOrder(const RecursiveClauseOrder& other) = delete;
        RecursiveClauseOrder& operator=(const RecursiveClauseOrder& other) = delete;
        
    private:
        void readGatesRecursive(GateAnalyzer &analyzer, Glucose::Lit output,
                                std::unordered_set<Glucose::Var> seenInputs,
                                std::unordered_set<Glucose::Var> seenOutputs);
        void backtrack(Gate &gate);
        
        std::vector<Glucose::Var> m_inputVariables;
        std::vector<Glucose::Lit> m_outputLitsOrdered;
        std::unordered_map<Glucose::Var, std::vector<const Glucose::Cl*>> m_clausesByOutput;
        Glucose::Var m_maxVar;
    };
    
    RecursiveClauseOrder::RecursiveClauseOrder() : ClauseOrder(), m_inputVariables({}),
    m_outputLitsOrdered({}), m_clausesByOutput({}), m_maxVar(-1) {
    }
    
    RecursiveClauseOrder::~RecursiveClauseOrder() {
    }
    
    const std::vector<Glucose::Var>& RecursiveClauseOrder::getInputVariables() const {
        return m_inputVariables;
    }
    
    const std::vector<Glucose::Lit>& RecursiveClauseOrder::getGateOutputsOrdered() const {
        return m_outputLitsOrdered;
    }
    
    const std::vector<const Glucose::Cl*>& RecursiveClauseOrder::getClauses(Glucose::Var variable) const {
        auto resultIter = m_clausesByOutput.find(variable);
        assert(resultIter != m_clausesByOutput.end());
        return resultIter->second;
    }
    
    unsigned int RecursiveClauseOrder::getAmountOfVars() const {
        return m_maxVar + 1;
    }
    
    void RecursiveClauseOrder::readGates(GateAnalyzer& analyzer) {
        std::unordered_set<Glucose::Var> seenInputs {}, seenOutputs{};
        
        for (auto rootClause : analyzer.getRoots()) {
            for (auto rootLit : *rootClause) {
                if (analyzer.getGate(rootLit).isDefined()) {
                    readGatesRecursive(analyzer, rootLit, seenInputs, seenOutputs);
                }
            }
        }
    }
    
    void RecursiveClauseOrder::readGatesRecursive(GateAnalyzer &analyzer, Glucose::Lit output,
                                                  std::unordered_set<Glucose::Var> seenInputs,
                                                  std::unordered_set<Glucose::Var> seenOutputs) {
        seenOutputs.insert(Glucose::var(output));
        
        auto &gate = analyzer.getGate(output);
        for (auto inputLit : gate.getInputs()) {
            auto inputVar = Glucose::var(inputLit);
            m_maxVar = std::max(m_maxVar, inputVar);
            
            if (analyzer.getGate(inputLit).isDefined()
                && seenOutputs.find(inputVar) == seenOutputs.end()) {
                readGatesRecursive(analyzer, inputLit, seenInputs, seenOutputs);
            }
            else if (seenInputs.find(inputVar) == seenInputs.end()) {
                seenInputs.insert(inputVar);
                m_inputVariables.push_back(inputVar);
            }
        }
        
        backtrack(gate);
    }
    
    void RecursiveClauseOrder::backtrack(Gate &gate) {
        Glucose::Lit usedOutput;
        Glucose::For *usedGateClauses;
        
        if (gate.hasNonMonotonousParent()
            || gate.getForwardClauses().size() <= gate.getBackwardClauses().size()) {
            usedOutput = ~gate.getOutput();
            usedGateClauses = &gate.getForwardClauses();
        }
        else {
            usedOutput = gate.getOutput();
            usedGateClauses = &gate.getBackwardClauses();
        }
        
        m_outputLitsOrdered.push_back(usedOutput);
        auto &clausesTarget = m_clausesByOutput[Glucose::var(usedOutput)];
        assert (clausesTarget.empty());
        clausesTarget.insert(clausesTarget.begin(), usedGateClauses->begin(), usedGateClauses->end());
    }
    
    std::unique_ptr<ClauseOrder> createDefaultClauseOrder() {
        return std::make_unique<RecursiveClauseOrder>();
    }
}
