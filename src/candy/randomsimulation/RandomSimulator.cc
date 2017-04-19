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

#include "RandomSimulator.h"

#include <cassert>

#include <utils/MemUtils.h>

#include "ClauseOrder.h"
#include "Partition.h"
#include "Randomization.h"
#include "SimulationVector.h"
#include "Propagation.h"

// TODO: documentation

namespace Candy {
    
    /* "interface" destructors */
    
    RandomSimulator::RandomSimulator() {
        
    }
    
    RandomSimulator::~RandomSimulator() {
        
    }
    
    
    RandomSimulatorBuilder::RandomSimulatorBuilder() {
        
    }
    
    RandomSimulatorBuilder::~RandomSimulatorBuilder() {
        
    }
    
    class BitparallelRandomSimulator : public RandomSimulator {
    public:
        BitparallelRandomSimulator(std::unique_ptr<ClauseOrder> clauseOrderStrat,
                                   std::unique_ptr<Partition> partitionStrat,
                                   std::unique_ptr<Randomization> randomizationStrat,
                                   std::unique_ptr<Propagation> propagationStrat,
                                   GateAnalyzer &gateAnalyzer,
                                   float reductionRateAbortThreshold);
        
        Conjectures run(unsigned int nSteps) override;
        Conjectures run() override;
        
        void ensureInitialized();
        
        
        virtual ~BitparallelRandomSimulator();
        BitparallelRandomSimulator(const BitparallelRandomSimulator& other) = delete;
        BitparallelRandomSimulator& operator=(const BitparallelRandomSimulator &other) = delete;
        
    private:
        bool isFurtherSimulationWorthwile();
        
        Conjectures runImpl(bool boundedRun, unsigned int nSteps);
        
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        std::unique_ptr<Propagation> m_propagationStrat;
        
        GateAnalyzer& m_gateAnalyzer;
        bool m_isInitialized;
        
        SimulationVectors m_simulationVectors;
        float m_abortThreshold;
    };
    
    BitparallelRandomSimulator::BitparallelRandomSimulator(std::unique_ptr<ClauseOrder> clauseOrderStrat,
                                                           std::unique_ptr<Partition> partitionStrat,
                                                           std::unique_ptr<Randomization> randomizationStrat,
                                                           std::unique_ptr<Propagation> propagationStrat,
                                                           GateAnalyzer &gateAnalyzer,
                                                           float reductionRateAbortThreshold)
    : RandomSimulator(), m_clauseOrderStrat(std::move(clauseOrderStrat)),
    m_partitionStrat(std::move(partitionStrat)),
    m_randomizationStrat(std::move(randomizationStrat)),
    m_propagationStrat(std::move(propagationStrat)),
    m_gateAnalyzer(gateAnalyzer),
    m_isInitialized(false),
    m_simulationVectors(),
    m_abortThreshold(reductionRateAbortThreshold)
    {
    }
    
    static std::vector<Glucose::Var> variables(const std::vector<Glucose::Lit>& literals) {
        std::vector<Glucose::Var> result;
        for (auto lit : literals) {
            result.push_back(var(lit));
        }
        return result;
    }
    
    void BitparallelRandomSimulator::ensureInitialized() {
        if (m_isInitialized) {
            return;
        }
        m_clauseOrderStrat->readGates(m_gateAnalyzer);
              
        m_simulationVectors.initialize(m_clauseOrderStrat->getAmountOfVars());
        m_partitionStrat->setVariables(variables(m_clauseOrderStrat->getGateOutputsOrdered()));
        m_isInitialized = true;
    }
    
    bool BitparallelRandomSimulator::isFurtherSimulationWorthwile() {
        return m_abortThreshold < 0.0f
               || m_partitionStrat->getPartitionReductionRate() > m_abortThreshold;
    }
    
    Conjectures BitparallelRandomSimulator::run() {
        assert (m_abortThreshold >= 0.0f);
        return runImpl(false, 0);
    }
    
    Conjectures BitparallelRandomSimulator::run(unsigned int nSteps) {
        assert (nSteps > 0);
        return runImpl(true, nSteps);
    }
    
    Conjectures BitparallelRandomSimulator::runImpl(bool boundedRun, unsigned int nSteps) {
        ensureInitialized();
        
        unsigned int realSteps = nSteps / (SimulationVector::VARSIMVECVARS);
        realSteps += (nSteps % SimulationVector::VARSIMVECVARS == 0 ? 0 : 1);
        
        auto& inputVars = m_clauseOrderStrat->getInputVariables();
        
        for (unsigned int step = 0; !boundedRun || step < realSteps; ++step) {
            m_randomizationStrat->randomize(m_simulationVectors, inputVars);
            m_propagationStrat->propagate(m_simulationVectors, *m_clauseOrderStrat);
            m_partitionStrat->update(m_simulationVectors);
            
            // Perform a minimum amount of simulation steps to let the randomization
            // work with different biases. TODO: ask the randomization object about
            // the concrete minimum of steps.
            if (step > 10 && !isFurtherSimulationWorthwile()) {
                break;
            }
        }
        
        return m_partitionStrat->getConjectures();
    }
    
    BitparallelRandomSimulator::~BitparallelRandomSimulator() {
    }
    
    
    /* TODO make the random simulator type a template argument */
    class BitparallelRandomSimulatorBuilder : public RandomSimulatorBuilder {
    public:
        BitparallelRandomSimulatorBuilder();
        
        BitparallelRandomSimulatorBuilder& withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat) override;
        BitparallelRandomSimulatorBuilder& withPartitionStrategy(std::unique_ptr<Partition> partitionStrat) override;
        BitparallelRandomSimulatorBuilder& withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat) override;
        BitparallelRandomSimulatorBuilder& withPropagationStrategy(std::unique_ptr<Propagation> propagationStrat) override;
        BitparallelRandomSimulatorBuilder& withGateAnalyzer(GateAnalyzer& gateAnalyzer) override;
        BitparallelRandomSimulatorBuilder& withReductionRateAbortThreshold(float threshold) override;
        BitparallelRandomSimulatorBuilder& withGateFilter(std::unique_ptr<GateFilter> filter) override;
        std::unique_ptr<RandomSimulator> build() override;


        
        virtual ~BitparallelRandomSimulatorBuilder();
        BitparallelRandomSimulatorBuilder(const BitparallelRandomSimulatorBuilder& other) = delete;
        BitparallelRandomSimulatorBuilder& operator=(const BitparallelRandomSimulatorBuilder& other) = delete;
        
    private:
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        std::unique_ptr<Propagation> m_propagationStrat;
        std::vector<std::unique_ptr<GateFilter>> m_gateFilters;
        GateAnalyzer *m_gateAnalyzer;
        float m_reductionRateAbortThreshold;
    };
    
    BitparallelRandomSimulatorBuilder::BitparallelRandomSimulatorBuilder()
    : RandomSimulatorBuilder(), m_clauseOrderStrat(nullptr) , m_partitionStrat(nullptr), m_randomizationStrat(nullptr), m_propagationStrat(nullptr), m_gateFilters(), m_gateAnalyzer(nullptr), m_reductionRateAbortThreshold(-1.0f) {
    }
    
    BitparallelRandomSimulatorBuilder::~BitparallelRandomSimulatorBuilder() {
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withGateFilter(std::unique_ptr<GateFilter> filter) {
        m_gateFilters.push_back(std::move(filter));
        return *this;
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat) {
        m_clauseOrderStrat = std::move(clauseOrderStrat);
        return *this;
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withPartitionStrategy(std::unique_ptr<Partition> partitionStrat) {
        m_partitionStrat = std::move(partitionStrat);
        return *this;
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat) {
        m_randomizationStrat = std::move(randomizationStrat);
        return *this;
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withPropagationStrategy(std::unique_ptr<Propagation> propagationStrat) {
        m_propagationStrat = std::move(propagationStrat);
        return *this;
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withGateAnalyzer(GateAnalyzer& gateAnalyzer) {
        m_gateAnalyzer = &gateAnalyzer;
        return *this;
    }
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withReductionRateAbortThreshold(float threshold) {
        assert (threshold > 0);
        m_reductionRateAbortThreshold = threshold;
        return *this;
    }

    
    std::unique_ptr<RandomSimulator> BitparallelRandomSimulatorBuilder::build() {
        if (m_gateAnalyzer == nullptr) {
            // TODO: do we have exception support?
            return nullptr;
        }
        
        if (m_clauseOrderStrat.get() == nullptr) {
            m_clauseOrderStrat = createNonrecursiveClauseOrder();
        }
        if (m_randomizationStrat.get() == nullptr) {
            auto positiveBiased = createRandomizationCyclicallyBiasedToTrue(1, 5, 1);
            auto negativeBiased = createRandomizationCyclicallyBiasedToFalse(1, 5, 1);
            m_randomizationStrat = alternateRandomizations(std::move(positiveBiased),
                                                           std::move(negativeBiased),
                                                           5);
            
        }
        if (m_partitionStrat.get() == nullptr) {
            m_partitionStrat = createDefaultPartition();
        }
        if (m_propagationStrat.get() == nullptr) {
            m_propagationStrat = createInputToOutputPropagation();
        }
        
        for (auto& gateFilter : m_gateFilters) {
            m_clauseOrderStrat->setGateFilter(std::move(gateFilter));
        }
        
        return backported_std::make_unique<BitparallelRandomSimulator>(std::move(m_clauseOrderStrat),
                                                                       std::move(m_partitionStrat),
                                                                       std::move(m_randomizationStrat),
                                                                       std::move(m_propagationStrat),
                                                                       *m_gateAnalyzer,
                                                                       m_reductionRateAbortThreshold);
    }
    

    
    std::unique_ptr<RandomSimulatorBuilder> createDefaultRandomSimulatorBuilder() {
        return backported_std::make_unique<BitparallelRandomSimulatorBuilder>();
    }
    
    std::unique_ptr<RandomSimulator> createDefaultRandomSimulator(GateAnalyzer& gateAnalyzer) {
        auto defaultBuilder = createDefaultRandomSimulatorBuilder();
        defaultBuilder->withGateAnalyzer(gateAnalyzer);
        return defaultBuilder->build();
    }
}
