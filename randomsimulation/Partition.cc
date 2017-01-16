#include "Partition.h"

#include <memory>

#include "SimulationVector.h"
#include "Conjectures.h"

namespace randsim {
    Partition::Partition() {
        
    }
    
    Partition::~Partition() {
        
    }
    
    class DefaultPartition : public Partition {
    public:
        DefaultPartition();
        
        void update(SimulationVectors &assignment) override;
        Conjectures getConjectures() override;
        bool isContinuationWorthwile() override;
        
        virtual ~DefaultPartition();
        DefaultPartition(const DefaultPartition& other) = delete;
        DefaultPartition& operator=(const DefaultPartition &other) = delete;
        
    private:
        void ensureInitialized();
        unsigned int m_round;
    };
    
    DefaultPartition::DefaultPartition() : Partition(), m_round(0) {
        
    }
    
    DefaultPartition::~DefaultPartition() {
        
    }
    
    void DefaultPartition::ensureInitialized() {
        if (m_round != 0) {
            return;
        }
        // TODO implementation
    }
    
    void DefaultPartition::update(SimulationVectors &assignment) {
        ensureInitialized();
        // TODO implementation
        ++m_round;
    }
    
    Conjectures DefaultPartition::getConjectures() {
        Conjectures result;
        // TODO implementation
        return result;
    }
    
    bool DefaultPartition::isContinuationWorthwile() {
        return true;
    }
    
    std::unique_ptr<Partition> createDefaultPartition() {
        return std::make_unique<DefaultPartition>();
    }
}
