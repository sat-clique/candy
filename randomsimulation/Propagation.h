#ifndef _7B650FD1_2ABE_456C_ABBF_B7339281B038_PROPAGATION_H
#define _7B650FD1_2ABE_456C_ABBF_B7339281B038_PROPAGATION_H

namespace randsim {
    class SimulationVectors;
    class ClauseOrder;
    
    class Propagation {
    public:
        virtual void propagate(SimulationVectors& assignment, ClauseOrder& clauseOrder);
        
        // todo: disable big three
    };
}


#endif
