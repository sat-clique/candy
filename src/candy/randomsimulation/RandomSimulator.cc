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

#include <candy/utils/MemUtils.h>
#include <candy/utils/Runtime.h>
#include <candy/frontend/CLIOptions.h>

#include "ClauseOrder.h"
#include "Partition.h"
#include "Randomization.h"
#include "SimulationVector.h"
#include "Propagation.h"

// TODO: documentation

namespace Candy {    
    BitparallelRandomSimulator::BitparallelRandomSimulator(std::unique_ptr<ClauseOrder> clauseOrderStrat,
        std::unique_ptr<Partition> partitionStrat,
        std::unique_ptr<Randomization> randomizationStrat, 
        std::unique_ptr<Propagation> propagationStrat,
        const GateAnalyzer& gateAnalyzer,
        float reductionRateAbortThreshold) :
    m_clauseOrderStrat(std::move(clauseOrderStrat)),
    m_partitionStrat(std::move(partitionStrat)),
    m_randomizationStrat(std::move(randomizationStrat)),
    m_propagationStrat(std::move(propagationStrat)),
    m_gateAnalyzer(gateAnalyzer),
    m_isInitialized(false),
    m_simulationVectors(),
    m_abortThreshold(reductionRateAbortThreshold)
    {
    }
    
    static std::vector<Candy::Var> variables(const std::vector<Candy::Lit>& literals) {
        std::vector<Candy::Var> result;
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
        return runImpl(false, 0, -1);
    }
    
    Conjectures BitparallelRandomSimulator::run(unsigned int nSteps) {
        assert (nSteps > 0);
        return runImpl(true, nSteps, -1);
    }
    
    Conjectures BitparallelRandomSimulator::run(unsigned int nSteps, double timeLimit) {
        assert (nSteps > 0);
        assert (timeLimit >= -1);
        return runImpl(true, nSteps, timeLimit);
    }
    
    Conjectures BitparallelRandomSimulator::runImpl(bool boundedRun, unsigned int nSteps, double timeLimit) {
        ensureInitialized();
        
        unsigned int realSteps = nSteps / (SimulationVector::VARSIMVECVARS);
        realSteps += (nSteps % SimulationVector::VARSIMVECVARS == 0 ? 0 : 1);
        
        auto& inputVars = m_clauseOrderStrat->getInputVariables();
        
        double startCPUTime = get_cpu_time();
        
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
            
            if (timeLimit > 0) {
                double currentCPUTime = get_cpu_time();
                if ((currentCPUTime - startCPUTime) > timeLimit) {
                    throw OutOfTimeException{};
                }
            }
        }
        
        return m_partitionStrat->getConjectures();
    }
    
    BitparallelRandomSimulator::~BitparallelRandomSimulator() { 
    }
    
    BitparallelRandomSimulatorBuilder::BitparallelRandomSimulatorBuilder(const GateAnalyzer& analyzer) : 
        m_clauseOrderStrat(nullptr) , 
        m_partitionStrat(nullptr), 
        m_randomizationStrat(nullptr), 
        m_propagationStrat(nullptr), 
        m_gateFilters(), 
        m_gateAnalyzer(analyzer), 
        m_reductionRateAbortThreshold(-1.0f) 
    {
        if (RandomSimulationOptions::opt_rs_abortbyrrat) {
            withReductionRateAbortThreshold(RandomSimulationOptions::opt_rs_rrat);
        }
        if (RandomSimulationOptions::opt_rs_filterGatesByNonmono) {
            withGateFilter(createNonmonotonousGateFilter(analyzer));
        }
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
    
    BitparallelRandomSimulatorBuilder& BitparallelRandomSimulatorBuilder::withReductionRateAbortThreshold(float threshold) {
        assert (threshold > 0);
        m_reductionRateAbortThreshold = threshold;
        return *this;
    }

    
    std::unique_ptr<BitparallelRandomSimulator> BitparallelRandomSimulatorBuilder::build() {
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
                                                                       m_gateAnalyzer,
                                                                       m_reductionRateAbortThreshold);
    }
}
