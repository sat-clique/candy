#include "RandomSimulator.h"

#include <cassert>

#include "ClauseOrder.h"
#include "Partition.h"
#include "Randomization.h"
#include "SimulationVector.h"
#include "Propagation.h"

#include "gates/GateAnalyzer.h"



namespace randsim {
    
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
                                   GateAnalyzer &gateAnalyzer);
        
        Conjectures run(unsigned int nSteps) override;
        void ensureInitialized();
        
        
        virtual ~BitparallelRandomSimulator();
        BitparallelRandomSimulator(const BitparallelRandomSimulator& other) = delete;
        BitparallelRandomSimulator& operator=(const BitparallelRandomSimulator &other) = delete;
        
    private:
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        std::unique_ptr<Propagation> m_propagationStrat;
        
        GateAnalyzer& m_gateAnalyzer;
        bool m_isInitialized;
        
        SimulationVectors m_simulationVectors;
    };
    
    BitparallelRandomSimulator::BitparallelRandomSimulator(std::unique_ptr<ClauseOrder> clauseOrderStrat,
                                                           std::unique_ptr<Partition> partitionStrat,
                                                           std::unique_ptr<Randomization> randomizationStrat,
                                                           std::unique_ptr<Propagation> propagationStrat,
                                                           GateAnalyzer &gateAnalyzer)
    : RandomSimulator(), m_clauseOrderStrat(std::move(clauseOrderStrat)),
    m_partitionStrat(std::move(partitionStrat)),
    m_randomizationStrat(std::move(randomizationStrat)),
    m_propagationStrat(std::move(propagationStrat)),
    m_gateAnalyzer(gateAnalyzer),
    m_isInitialized(false),
    m_simulationVectors()
    {
    }
    
    void BitparallelRandomSimulator::ensureInitialized() {
        if (m_isInitialized) {
            return;
        }
        m_clauseOrderStrat->readGates(m_gateAnalyzer);
        m_simulationVectors.initialize(m_clauseOrderStrat->getAmountOfVars());
        m_isInitialized = true;
    }
    
    Conjectures BitparallelRandomSimulator::run(unsigned int nSteps) {
        ensureInitialized();
        
        assert (nSteps % SimulationVector::VARSIMVECVARS == 0);
        unsigned int realSteps = nSteps / (SimulationVector::VARSIMVECVARS);
        
        auto& inputVars = m_clauseOrderStrat->getInputVariables();
        
        for (size_t step = 0; step < realSteps; ++step) {
            m_randomizationStrat->randomize(m_simulationVectors, inputVars);
            m_propagationStrat->propagate(m_simulationVectors, *m_clauseOrderStrat);
            m_partitionStrat->update(m_simulationVectors);
        }
        
        return m_partitionStrat->getConjectures();
    }
    
    BitparallelRandomSimulator::~BitparallelRandomSimulator() {
    }
    
    
    
    class BitparallelRandomSimulatorBuilder : public RandomSimulatorBuilder {
    public:
        BitparallelRandomSimulatorBuilder();
        
        BitparallelRandomSimulatorBuilder& withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat) override;
        BitparallelRandomSimulatorBuilder& withPartitionStrategy(std::unique_ptr<Partition> partitionStrat) override;
        BitparallelRandomSimulatorBuilder& withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat) override;
        BitparallelRandomSimulatorBuilder& withPropagationStrategy(std::unique_ptr<Propagation> propagationStrat) override;
        BitparallelRandomSimulatorBuilder& withGateAnalyzer(GateAnalyzer& gateAnalyzer) override;
        std::unique_ptr<RandomSimulator> build() override;


        
        virtual ~BitparallelRandomSimulatorBuilder();
        BitparallelRandomSimulatorBuilder(const BitparallelRandomSimulatorBuilder& other) = delete;
        BitparallelRandomSimulatorBuilder& operator=(const BitparallelRandomSimulatorBuilder& other) = delete;
        
    private:
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        std::unique_ptr<Propagation> m_propagationStrat;
        GateAnalyzer *m_gateAnalyzer;
    };
    
    BitparallelRandomSimulatorBuilder::BitparallelRandomSimulatorBuilder()
    : RandomSimulatorBuilder(), m_clauseOrderStrat(nullptr) , m_partitionStrat(nullptr), m_randomizationStrat(nullptr), m_propagationStrat(nullptr), m_gateAnalyzer(nullptr) {
    }
    
    BitparallelRandomSimulatorBuilder::~BitparallelRandomSimulatorBuilder() {
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

    
    std::unique_ptr<RandomSimulator> BitparallelRandomSimulatorBuilder::build() {
        if (m_gateAnalyzer == nullptr) {
            // TODO: do we have exception support?
            return nullptr;
        }
        
        if (m_clauseOrderStrat.get() == nullptr) {
            m_clauseOrderStrat = createDefaultClauseOrder();
        }
        if (m_randomizationStrat.get() == nullptr) {
            m_randomizationStrat = std::make_unique<Randomization>();
        }
        if (m_partitionStrat.get() == nullptr) {
            m_partitionStrat = std::make_unique<Partition>();
        }
        if (m_propagationStrat.get() == nullptr) {
            m_propagationStrat = std::make_unique<Propagation>();
        }
        
        // TODO: read and order clauses
        
        return std::make_unique<BitparallelRandomSimulator>(std::move(m_clauseOrderStrat),
                                                            std::move(m_partitionStrat),
                                                            std::move(m_randomizationStrat),
                                                            std::move(m_propagationStrat),
                                                            *m_gateAnalyzer);
    }
    

    
    std::unique_ptr<RandomSimulatorBuilder> createDefaultRandomSimulatorBuilder() {
        return std::make_unique<BitparallelRandomSimulatorBuilder>();
    }
    
    std::unique_ptr<RandomSimulator> createDefaultRandomSimulator() {
        auto defaultBuilder = createDefaultRandomSimulatorBuilder();
        return defaultBuilder->build();
    }
}
