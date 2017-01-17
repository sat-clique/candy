#ifndef _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H
#define _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H

#include <memory>
#include <vector>

#include <core/SolverTypes.h>

namespace randsim {
    class SimulationVectors;
    class Conjectures;
    
    class Partition {
    public:
        virtual void setVariables(const std::vector<Glucose::Var> &variables) = 0;
        virtual void update(const SimulationVectors &assignment) = 0;
        virtual Conjectures getConjectures() = 0;
        virtual float getPartitionReductionRate() = 0;
        
        Partition();
        virtual ~Partition();
        Partition(const Partition& other) = delete;
        Partition& operator=(const Partition &other) = delete;
    };
    
    class CompressionScheduleStrategy {
    public:
        virtual bool shouldCompress(unsigned int m_round) = 0;
        
        CompressionScheduleStrategy();
        virtual ~CompressionScheduleStrategy();
        CompressionScheduleStrategy(const CompressionScheduleStrategy& other) = delete;
        CompressionScheduleStrategy& operator=(const CompressionScheduleStrategy &other) = delete;
    };
    
    std::unique_ptr<Partition> createDefaultPartition();
    std::unique_ptr<Partition> createDefaultPartition(std::unique_ptr<CompressionScheduleStrategy> compressionScheduleStrategy);
    
    std::unique_ptr<CompressionScheduleStrategy> createLinearCompressionScheduleStrategy(unsigned int freq);
    std::unique_ptr<CompressionScheduleStrategy> createLogCompressionScheduleStrategy();
    std::unique_ptr<CompressionScheduleStrategy> createNullCompressionScheduleStrategy();
}

#endif
