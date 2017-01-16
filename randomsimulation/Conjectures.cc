#include "Conjectures.h"

namespace randsim {
    void EquivalenceConjecture::addLit(Glucose::Lit lit) {
        m_lits.push_back(lit);
    }
    
    const std::vector<Glucose::Lit> EquivalenceConjecture::getLits() const {
        return m_lits;
    }
    
    BackboneConjecture::BackboneConjecture(Glucose::Lit lit) : m_lit(lit) {
        
    }
    
    Glucose::Lit BackboneConjecture::getLit() {
        return m_lit;
    }
    
    void Conjectures::addEquivalence(randsim::EquivalenceConjecture &conj) {
        m_equivalences.push_back(conj);
    }
    
    void Conjectures::addBackbone(randsim::BackboneConjecture &conj) {
        m_backbones.push_back(conj);
    }
    
    const std::vector<EquivalenceConjecture> Conjectures::getEquivalences() const {
        return m_equivalences;
    }
    
    const std::vector<BackboneConjecture> Conjectures::getBackbones() const {
        return m_backbones;
    }
}
