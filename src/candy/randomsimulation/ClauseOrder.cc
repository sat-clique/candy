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

// TODO: documentation

#include "ClauseOrder.h"

#include <memory>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include <candy/gates/GateAnalyzer.h>
#include <candy/gates/GateDFSTraversal.h>
#include <candy/core/SolverTypes.h>

#include <candy/utils/MemUtils.h>

namespace Candy {
    ClauseOrder::ClauseOrder() {
        
    }
    
    ClauseOrder::~ClauseOrder() {
        
    }
    
    GateFilter::GateFilter() {
        
    }
    
    GateFilter::~GateFilter() {
        
    }
    
    /**
     * \class ClauseOrderImplBase
     *
     * \ingroup RandomSimulation
     *
     * \brief The base class for ClauseOrder implementations
     *
     * This class provides the common infrastructure of ClauseOrder
     * implementations. "Final" implementations need only implement
     * the readGates() method calling backtrack() in the correct order
     * and setting up m_inputVariables as well as m_maxVar.
     */
    class ClauseOrderImplBase : public ClauseOrder {
    public:
        void readGates(const GateAnalyzer& analyzer) override = 0;
        const std::vector<Candy::Var> &getInputVariables() const override;
        const std::vector<Candy::Lit> &getGateOutputsOrdered() const override;
        const std::vector<const Cl*> &getClauses(Candy::Var variable) const override;
        void setGateFilter(std::unique_ptr<GateFilter> gateFilter) override;
        unsigned int getAmountOfVars() const override;
        
        ClauseOrderImplBase();
        virtual ~ClauseOrderImplBase();
        ClauseOrderImplBase(const ClauseOrderImplBase& other) = delete;
        ClauseOrderImplBase& operator=(const ClauseOrderImplBase& other) = delete;
        
    protected:
        void backtrack(const GateAnalyzer& analyzer, const Gate &gate);
        std::vector<const Cl*> &createClauseStorage(Candy::Var variable);
        
        std::vector<Candy::Var> m_inputVariables;
        std::vector<Candy::Lit> m_outputLitsOrdered;
        std::vector<std::unique_ptr<std::vector<const Cl*>>> m_clausesByOutput;
        Candy::Var m_maxVar;
        
        std::unique_ptr<GateFilter> m_gateFilter;
        std::unordered_set<Candy::Var> m_enabledOutputs;
        bool m_filteringEnabled;
    };
    
    ClauseOrderImplBase::ClauseOrderImplBase() : ClauseOrder(), m_inputVariables({}),
    m_outputLitsOrdered({}), m_clausesByOutput(), m_maxVar(-1), m_gateFilter(nullptr),
    m_enabledOutputs({}), m_filteringEnabled(false) {
        m_clausesByOutput.resize(65536);
    }
    
    ClauseOrderImplBase::~ClauseOrderImplBase() {
    }
    
    void ClauseOrderImplBase::setGateFilter(std::unique_ptr<GateFilter> filter) {
        m_gateFilter = std::move(filter);
        m_filteringEnabled = true;
    }
    
    const std::vector<Candy::Var>& ClauseOrderImplBase::getInputVariables() const {
        return m_inputVariables;
    }
    
    const std::vector<Candy::Lit>& ClauseOrderImplBase::getGateOutputsOrdered() const {
        return m_outputLitsOrdered;
    }
    
    std::vector<const Cl*>& ClauseOrderImplBase::createClauseStorage(Candy::Var variable) {
        assert(variable >= 0);
        if(static_cast<size_t>(variable) >= m_clausesByOutput.size()) {
            m_clausesByOutput.resize(2*variable + 1);
        }
        assert(m_clausesByOutput[variable] == nullptr);
        
        m_clausesByOutput[variable].reset(new std::vector<const Cl*>{});
        
        return *m_clausesByOutput[variable];
    }
    
    const std::vector<const Cl*>& ClauseOrderImplBase::getClauses(Candy::Var variable) const {
        assert(variable >= 0);
        assert(static_cast<size_t>(variable) < m_clausesByOutput.size());
        assert(m_clausesByOutput[variable] != nullptr);
        return *m_clausesByOutput[variable];
    }
    
    unsigned int ClauseOrderImplBase::getAmountOfVars() const {
        return m_maxVar + 1;
    }
    
    void ClauseOrderImplBase::backtrack(const GateAnalyzer& analyzer, const Gate &gate) {
        if (m_filteringEnabled) {
            // backtrack the gate only if
            //  - its output is an enabled variable
            //  - for none of its input variables i, the following holds: i is an output variable of a gate and i is disabled.
            
            Candy::Var outputVar = gate.getOutput().var();
            
            if (m_enabledOutputs.find(outputVar) == m_enabledOutputs.end()) {
                return; // no backtracking
            }
            
            for (auto inputLit : gate.getInputs()) {
                if (analyzer.getResult().getGate(inputLit).isDefined()
                    && m_enabledOutputs.find(inputLit.var()) == m_enabledOutputs.end()) {
                    return; // no backtracking
                }
            }
        }
        
        Candy::Lit usedOutput;
        const For *usedGateClauses;
        
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
        auto &clausesTarget = createClauseStorage(usedOutput.var());
        assert (clausesTarget.empty());
        clausesTarget.insert(clausesTarget.begin(), usedGateClauses->begin(), usedGateClauses->end());
    }
    
    void ClauseOrderImplBase::readGates(const GateAnalyzer& analyzer) {
        (void)analyzer;
        m_inputVariables.clear();
        m_maxVar = -1;
        m_clausesByOutput.clear();
        m_outputLitsOrdered.clear();
        
        if (m_filteringEnabled) {
            m_enabledOutputs = m_gateFilter->getEnabledOutputVars();
        }
    }
    
    
    /**
     * \class RecursiveClauseOrder
     *
     * \ingroup RandomSimulation
     *
     * \brief A ClauseOrder implementation based on recursion.
     */
    class RecursiveClauseOrder : public ClauseOrderImplBase {
    public:
        void readGates(const GateAnalyzer& analyzer) override;
        
        RecursiveClauseOrder();
        virtual ~RecursiveClauseOrder();
        RecursiveClauseOrder(const RecursiveClauseOrder& other) = delete;
        RecursiveClauseOrder& operator=(const RecursiveClauseOrder& other) = delete;
        
    private:
        void readGatesRecursive(const GateAnalyzer &analyzer, Candy::Lit output,
                                std::unordered_set<Candy::Var>& seenInputs,
                                std::unordered_set<Candy::Var>& seenOutputs);
    };
    
    RecursiveClauseOrder::RecursiveClauseOrder() : ClauseOrderImplBase() {
        
    }
    
    RecursiveClauseOrder::~RecursiveClauseOrder() {
        
    }
    
    void RecursiveClauseOrder::readGates(const GateAnalyzer& analyzer) {
        ClauseOrderImplBase::readGates(analyzer);
        
        if (analyzer.getResult().getGateCount() == 0) {
            return;
        }
        
        std::unordered_set<Candy::Var> seenInputs {}, seenOutputs{};
        
        for (auto rootClause : analyzer.getResult().getRoots()) {
            for (auto rootLit : *rootClause) {
                m_maxVar = std::max(m_maxVar, rootLit.var());
                if (analyzer.getResult().getGate(rootLit).isDefined()) {
                    readGatesRecursive(analyzer, rootLit, seenInputs, seenOutputs);
                }
            }
        }
    }
    
    void RecursiveClauseOrder::readGatesRecursive(const GateAnalyzer &analyzer, Candy::Lit output,
                                                  std::unordered_set<Candy::Var>& seenInputs,
                                                  std::unordered_set<Candy::Var>& seenOutputs) {
        seenOutputs.insert(output.var());
        
        auto &gate = analyzer.getResult().getGate(output);
        for (auto inputLit : gate.getInputs()) {
            auto inputVar = inputLit.var();
            m_maxVar = std::max(m_maxVar, inputVar);
            
            if (analyzer.getResult().getGate(inputLit).isDefined()
                && seenOutputs.find(inputVar) == seenOutputs.end()) {
                readGatesRecursive(analyzer, inputLit, seenInputs, seenOutputs);
            }
            else if (seenOutputs.find(inputVar) == seenOutputs.end()
                     && seenInputs.find(inputVar) == seenInputs.end()) {
                seenInputs.insert(inputVar);
                m_inputVariables.push_back(inputVar);
            }
        }
        
        assert(seenInputs.find(output.var()) == seenInputs.end());
        
        backtrack(analyzer, gate);
    }
    
    std::unique_ptr<ClauseOrder> createRecursiveClauseOrder() {
        return backported_std::make_unique<RecursiveClauseOrder>();
    }
    
    
    /**
     * \class NonrecursiveClauseOrder
     *
     * \ingroup RandomSimulation
     *
     * \brief An experimental ClauseOrder implementation using the GateDFSTraversal.
     */
    class NonrecursiveClauseOrder : public ClauseOrderImplBase {
    public:
        void readGates(const GateAnalyzer& analyzer) override;
        
        NonrecursiveClauseOrder();
        virtual ~NonrecursiveClauseOrder();
        NonrecursiveClauseOrder(const NonrecursiveClauseOrder& other) = delete;
        NonrecursiveClauseOrder& operator=(const NonrecursiveClauseOrder& other) = delete;
    };
    
    NonrecursiveClauseOrder::NonrecursiveClauseOrder() {
        
    }
    
    NonrecursiveClauseOrder::~NonrecursiveClauseOrder() {
        
    }
    
    struct DFSGateCollector {
        std::vector<const Gate*> backtrackSequence{};
        std::vector<Var> inputs{};
        Var maxVar = -1;
        
        void init(size_t gateCount) {
            backtrackSequence.reserve(gateCount);
        }
        
        void backtrack(const Gate* g) {
            maxVar = std::max(maxVar, g->getOutput().var());
            backtrackSequence.push_back(g);
        }
        
        void collect(const Gate* g) {
            maxVar = std::max(maxVar, g->getOutput().var());
        }
        
        void collectInput(Var v) {
            maxVar = std::max(maxVar, v);
            inputs.push_back(v);
        }
        
        bool pruneAt(const Gate& g) {
            (void)g;
            return false;
        }
        
        void finished() {
        }
        
        DFSGateCollector() = default;
        DFSGateCollector(DFSGateCollector&& other) = default;
        DFSGateCollector& operator=(DFSGateCollector&& other) = default;
    };
    
    void NonrecursiveClauseOrder::readGates(const GateAnalyzer& analyzer) {
        DFSGateCollector orderedGates = traverseDFS<DFSGateCollector>(analyzer.getGateProblem());
        m_inputVariables = std::move(orderedGates.inputs);
        m_maxVar = orderedGates.maxVar;
        for (auto gate : orderedGates.backtrackSequence) {
            backtrack(analyzer, *gate);
        }
    }
    
    std::unique_ptr<ClauseOrder> createNonrecursiveClauseOrder() {
        return backported_std::make_unique<NonrecursiveClauseOrder>();
    }
    
    
    /**
     * \class NonmonotonousGateFilter
     * 
     * \ingroup RandomSimulation
     *
     * \brief A gate filter marking exactly the outputs of nonmonotonously nested gates as enabled.
     */
    class NonmonotonousGateFilter : public GateFilter {
    public:
        std::unordered_set<Candy::Var> getEnabledOutputVars() override;
        
        explicit NonmonotonousGateFilter(const GateAnalyzer &analyzer);
        virtual ~NonmonotonousGateFilter();
        NonmonotonousGateFilter(const NonmonotonousGateFilter& other) = delete;
        NonmonotonousGateFilter& operator=(const NonmonotonousGateFilter& other) = delete;
        
    private:
        const GateAnalyzer &m_analyzer;
    };
    
    NonmonotonousGateFilter::NonmonotonousGateFilter(const GateAnalyzer &analyzer)
    : GateFilter(),
    m_analyzer(analyzer) {
    }
    
    NonmonotonousGateFilter::~NonmonotonousGateFilter() {
    }
    
    std::unordered_set<Candy::Var> NonmonotonousGateFilter::getEnabledOutputVars() {
        std::unordered_set<Candy::Var> result;
        
        // TODO: find a more direct way for iterating over the gates
        auto topo = getTopoOrder(m_analyzer.getGateProblem());
        for (auto outputVar : topo.getOutputsOrdered()) {
            auto& gate = m_analyzer.getResult().getGate(Lit(outputVar, 1));
            if(gate.hasNonMonotonousParent()) {
                result.insert(outputVar);
            }
        }
        
        return result;
    }
    
    
    
    std::unique_ptr<GateFilter> createNonmonotonousGateFilter(const GateAnalyzer &analyzer) {
        return backported_std::make_unique<NonmonotonousGateFilter>(analyzer);
    }
}
