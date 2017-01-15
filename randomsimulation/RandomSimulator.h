#ifndef _E8029478_2FCD_4B07_9B47_58F51AA1C2FE_RANDOMSIMULATOR_H
#define _E8029478_2FCD_4B07_9B47_58F51AA1C2FE_RANDOMSIMULATOR_H

#include <memory>

class GateAnalyzer;

namespace randsim {
    class ClauseOrder;
    class Partition;
    class Randomization;
    
    class RandomSimulator {
    public:
        RandomSimulator();
        
        virtual void run(unsigned int nSteps) = 0;
        virtual ~RandomSimulator();
        
        RandomSimulator(const RandomSimulator &other) = delete;
        RandomSimulator& operator=(const RandomSimulator &other) = delete;
    };
    
    class RandomSimulatorBuilder {
    public:
        virtual RandomSimulatorBuilder& withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat) = 0;
        virtual RandomSimulatorBuilder& withPartitionStrategy(std::unique_ptr<Partition> partitionStrat) = 0;
        virtual RandomSimulatorBuilder& withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat) = 0;
        virtual RandomSimulatorBuilder& withGateAnalyzer(GateAnalyzer& gateAnalyzer) = 0;
        virtual std::unique_ptr<RandomSimulator> build() = 0;
        
        RandomSimulatorBuilder();
        virtual ~RandomSimulatorBuilder();
        RandomSimulatorBuilder(const RandomSimulatorBuilder& other) = delete;
        RandomSimulatorBuilder& operator=(const RandomSimulatorBuilder &other) = delete;
    };
    
    std::unique_ptr<RandomSimulator> createDefaultRandomSimulator();
    std::unique_ptr<RandomSimulatorBuilder> createDefaultRandomSimulatorBuilder();
}
#endif
