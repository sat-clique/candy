#ifndef _AAE7E9E3_CD0B_407F_8794_C5A8B42B678A_CONJECTURES_H
#define _AAE7E9E3_CD0B_407F_8794_C5A8B42B678A_CONJECTURES_H

#include <core/SolverTypes.h>

namespace randsim {
    class EquivalenceConjecture {
    public:
        void addLit(Glucose::Lit lit);
        const std::vector<Glucose::Lit> getLits() const;
        
    private:
        std::vector<Glucose::Lit> m_lits {};
    };
    
    class BackboneConjecture {
    public:
        BackboneConjecture(Glucose::Lit lit);
        Glucose::Lit getLit();
        
    private:
        Glucose::Lit m_lit;
    };
    
    class Conjectures {
    public:
        const std::vector<EquivalenceConjecture> &getEquivalences() const;
        const std::vector<BackboneConjecture> &getBackbones() const;
        
        void addEquivalence(EquivalenceConjecture &conj);
        void addBackbone(BackboneConjecture &conj);
        
    private:
        std::vector<EquivalenceConjecture> m_equivalences {};
        std::vector<BackboneConjecture> m_backbones {};
    };
}


#endif
