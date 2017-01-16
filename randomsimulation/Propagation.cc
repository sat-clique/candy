#include "Propagation.h"

#include "ClauseOrder.h"
#include "SimulationVector.h"

namespace randsim {
    Propagation::~Propagation() {
        
    }
    
    Propagation::Propagation() {
        
    }
    
    
    class InputToOutputPropagation : public Propagation {
    public:
        void propagate(SimulationVectors& assignment, ClauseOrder& clauseOrder) override;
        
        InputToOutputPropagation();
        virtual ~InputToOutputPropagation();
        InputToOutputPropagation(const InputToOutputPropagation& other) = delete;
        InputToOutputPropagation& operator=(const InputToOutputPropagation& other) = delete;
    };
    
    InputToOutputPropagation::InputToOutputPropagation() {
        
    }
    
    InputToOutputPropagation::~InputToOutputPropagation() {
        
    }
    
    AlignedSimVector createVarSimVector(bool ones) {
        AlignedSimVector result;
        for (unsigned int i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
            result.vars[i] = 0ull;
            if (ones) {
                result.vars[i] = ~result.vars[i]; //todo: use constant
            }
        }
        return result;
    };
    
    void InputToOutputPropagation::propagate(SimulationVectors &assignment,
                                             ClauseOrder &clauseOrder) {
        
        for (auto outputLit : clauseOrder.getGateOutputsOrdered()) {
            AlignedSimVector allSat = createVarSimVector(true);
            auto& gateClauses = clauseOrder.getClauses(outputLit);
            auto outputVar = Glucose::var(outputLit);
            
            for (auto clause : gateClauses) {
                AlignedSimVector satAssgn = createVarSimVector(false);
                for (auto literal : clause) {
                    AlignedSimVector* varAssgn = &(assignment.get(Glucose::var(literal)));
                    varAssgn = reinterpret_cast<AlignedSimVector*>(
                                __builtin_assume_aligned(varAssgn, RANDSIM_ALIGNMENT));
                    
                    if (Glucose::var(literal) == outputVar) {
                        continue;
                    }
                    
                    if (Glucose::sign(literal) == true) {
                        satAssgn |= *varAssgn;
                    }
                    else {
                        satAssgn |= ~(*varAssgn);
                    }
                }
                allSat &= satAssgn;
            }
            
            AlignedSimVector* outputAssgn = &(assignment.get(outputVar));
            outputAssgn = reinterpret_cast<AlignedSimVector*>(
                            __builtin_assume_aligned(outputAssgn, RANDSIM_ALIGNMENT));

            if (Glucose::sign(outputLit) == true) {
                *outputAssgn = ~allSat;
            }
            else {
                *outputAssgn = allSat;
            }
        }
    }
    
    
    std::unique_ptr<Propagation> createInputToOutputPropagation() {
        return std::make_unique<InputToOutputPropagation>();
    }
}
