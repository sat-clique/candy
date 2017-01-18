#ifndef _1EC0AC34_0ABE_4820_9A41_CD888D51FC30_TESTGATESTRUCTURE_H
#define _1EC0AC34_0ABE_4820_9A41_CD888D51FC30_TESTGATESTRUCTURE_H

#include <memory>
#include <vector>
#include <core/SolverTypes.h>
#include <core/CNFProblem.h>

namespace randsim {
    class GateStructureBuilder {
    public:
        virtual GateStructureBuilder& withAnd(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) = 0;
        virtual GateStructureBuilder& withOr(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) = 0;
        virtual GateStructureBuilder& withXor(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) = 0;
        virtual std::unique_ptr<Glucose::CNFProblem, void(*)(Glucose::CNFProblem*)> build() = 0;
        
        GateStructureBuilder();
        virtual ~GateStructureBuilder();
        GateStructureBuilder(const GateStructureBuilder& other) = delete;
        GateStructureBuilder& operator=(const GateStructureBuilder& other) = delete;
    };
    
    std::unique_ptr<GateStructureBuilder> createGateStructureBuilder();
}

#endif
