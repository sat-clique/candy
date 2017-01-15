#include "RandomSimulator.h"

#include "ClauseOrder.h"
#include "Partition.h"
#include "Randomization.h"
#include "SimulationVector.h"

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
                                   GateAnalyzer &gateAnalyzer);
        
        void run(unsigned int nSteps) override;
        
        
        virtual ~BitparallelRandomSimulator();
        BitparallelRandomSimulator(const BitparallelRandomSimulator& other) = delete;
        BitparallelRandomSimulator& operator=(const BitparallelRandomSimulator &other) = delete;
        
    private:
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat;
        std::unique_ptr<Partition> m_partitionStrat;
        std::unique_ptr<Randomization> m_randomizationStrat;
        GateAnalyzer& m_gateAnalyzer;
    };
    
    BitparallelRandomSimulator::~BitparallelRandomSimulator() {
    }
    
    
    
    class BitparallelRandomSimulatorBuilder : public RandomSimulatorBuilder {
    public:
        BitparallelRandomSimulatorBuilder();
        
        BitparallelRandomSimulatorBuilder& withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat) override;
        BitparallelRandomSimulatorBuilder& withPartitionStrategy(std::unique_ptr<Partition> partitionStrat) override;
        BitparallelRandomSimulatorBuilder& withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat) override;
        BitparallelRandomSimulatorBuilder& withGateAnalyzer(GateAnalyzer& gateAnalyzer) override;
        std::unique_ptr<RandomSimulator> build() override;
        
        
        virtual ~BitparallelRandomSimulatorBuilder();
        BitparallelRandomSimulatorBuilder(const BitparallelRandomSimulatorBuilder& other) = delete;
        BitparallelRandomSimulatorBuilder& operator=(const BitparallelRandomSimulatorBuilder& other) = delete;
        
    private:
        std::unique_ptr<ClauseOrder> m_clauseOrderStrat{};
        std::unique_ptr<Partition> m_partitionStrat{};
        std::unique_ptr<Randomization> m_randomizationStrat{};
        GateAnalyzer *m_gateAnalyzer;
    };
    
    BitparallelRandomSimulatorBuilder::BitparallelRandomSimulatorBuilder()
    : RandomSimulatorBuilder(), m_clauseOrderStrat(nullptr) , m_partitionStrat(nullptr), m_randomizationStrat(nullptr), m_gateAnalyzer(nullptr) {
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
            m_clauseOrderStrat = std::make_unique<ClauseOrder>();
        }
        if (m_randomizationStrat.get() == nullptr) {
            m_randomizationStrat = std::make_unique<Randomization>();
        }
        if (m_partitionStrat.get() == nullptr) {
            m_partitionStrat = std::make_unique<Partition>();
        }
        
        // TODO: read and order clauses
        
        return std::make_unique<BitparallelRandomSimulator>(std::move(m_clauseOrderStrat),
                                                            std::move(m_partitionStrat),
                                                            std::move(m_randomizationStrat),
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
