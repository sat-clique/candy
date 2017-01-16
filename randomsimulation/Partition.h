#ifndef _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H
#define _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H

#include <memory>

namespace randsim {
    class SimulationVectors;
    class Conjectures;
    
    class Partition {
    public:
        virtual void update(SimulationVectors &assignment) = 0;
        virtual Conjectures getConjectures() = 0;
        virtual bool isContinuationWorthwile() = 0;
        
        Partition();
        virtual ~Partition();
        Partition(const Partition& other) = delete;
        Partition& operator=(const Partition &other) = delete;
    };
    
    std::unique_ptr<Partition> createDefaultPartition();
}

#endif
