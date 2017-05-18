/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

// TODO: documentation

#include "Propagation.h"

#include <utils/MemUtils.h>

#include "ClauseOrder.h"
#include "SimulationVector.h"

#include <limits>

namespace Candy {
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
            result.vars[i] = ones ? std::numeric_limits<SimulationVector::varsimvec_field_t>::max() : 0ull;
        }
        return result;
    };
    
    void InputToOutputPropagation::propagate(SimulationVectors &assignment,
                                             ClauseOrder &clauseOrder) {
        auto onesVector = createVarSimVector(true);
        auto zeroesVector = createVarSimVector(false);
        
        for (auto outputLit : clauseOrder.getGateOutputsOrdered()) {
            AlignedSimVector allSat = onesVector;
            auto outputVar = var(outputLit);
            auto& gateClauses = clauseOrder.getClauses(outputVar);

            for (auto clause : gateClauses) {
                AlignedSimVector satAssgn = zeroesVector;
                for (auto literal : *clause) {
                    if (var(literal) == outputVar) {
                        continue;
                    }
                    
                    AlignedSimVector* varAssgn = &(assignment.get(var(literal)));
                    
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
                    varAssgn = reinterpret_cast<AlignedSimVector*>(
                                __builtin_assume_aligned(varAssgn, RANDSIM_ALIGNMENT));
#elif defined(_MSC_VER)
#pragma message ( "Warning: auto-vectorization not supported yet" )
#else
#error Unsupported compiler.
#endif
                    
                    if (sign(literal) == true) {
                        satAssgn |= *varAssgn;
                    }
                    else {
                        satAssgn |= ~(*varAssgn);
                    }
                }
                allSat &= satAssgn;
            }
            
            AlignedSimVector* outputAssgn = &(assignment.get(outputVar));
            
#if defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
            outputAssgn = reinterpret_cast<AlignedSimVector*>(
                            __builtin_assume_aligned(outputAssgn, RANDSIM_ALIGNMENT));
#elif defined(_MSC_VER)
#pragma message ( "Warning: auto-vectorization not supported yet" )
#else
#error Unsupported compiler.
#endif

            if (sign(outputLit) == true) {
                *outputAssgn = ~allSat;
            }
            else {
                *outputAssgn = allSat;
            }
        }
    }
    
    
    std::unique_ptr<Propagation> createInputToOutputPropagation() {
        return backported_std::make_unique<InputToOutputPropagation>();
    }
}
