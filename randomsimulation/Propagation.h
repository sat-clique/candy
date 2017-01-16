#ifndef _7B650FD1_2ABE_456C_ABBF_B7339281B038_PROPAGATION_H
#define _7B650FD1_2ABE_456C_ABBF_B7339281B038_PROPAGATION_H

#include <memory>

namespace randsim {
    class SimulationVectors;
    class ClauseOrder;
    
    class Propagation {
    public:
        virtual void propagate(SimulationVectors& assignment, ClauseOrder& clauseOrder) = 0;
        
        Propagation();
        virtual ~Propagation();
        Propagation(const Propagation& other) = delete;
        Propagation& operator=(const Propagation& other) = delete;
    };
    
    std::unique_ptr<Propagation> createInputToOutputPropagation();
}


#endif
