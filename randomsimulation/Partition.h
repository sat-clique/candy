#ifndef _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H
#define _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H

namespace randsim {
    class SimulationVectors;
    class Conjectures;
    
    class Partition {
    public:
        virtual void update(SimulationVectors &assignment);
        virtual Conjectures getConjectures();
        // todo: disable big three
    };
}

#endif
