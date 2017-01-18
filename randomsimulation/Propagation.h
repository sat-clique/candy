/* Copyright (c) 2017 Felix Kutzner
 
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

#ifndef X_7B650FD1_2ABE_456C_ABBF_B7339281B038_PROPAGATION_H
#define X_7B650FD1_2ABE_456C_ABBF_B7339281B038_PROPAGATION_H

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
