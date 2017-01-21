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

#ifndef X_0F5ABD44_AD19_4EAC_8743_CD4D875C9AFE_RANDOMIZATION_H
#define X_0F5ABD44_AD19_4EAC_8743_CD4D875C9AFE_RANDOMIZATION_H

#include <vector>
#include <memory>

#include "SimulationVector.h"

namespace randsim {
    class SimulationVectors;
    
    /**
     * \class Randomization
     *
     * \ingroup RandomSimulation
     *
     * \brief The randomization interface.
     *
     * Randomization objects are responsible for simulation vector (i.e. variable
     * assignment) randomization.
     *
     * Usage example: build a RandomSimulation object with this strategy.
     */
    class Randomization {
    public:
        /**
         * Randomizes the simulation vectors with the given indices.
         *
         * \param simVectors The simulation vectors.
         * \param indices The indices of the simulation vectors to be randomized.
         */
        virtual void randomize(SimulationVectors &simVectors,
                               std::vector<SimulationVectors::index_t> indices) = 0;
        
        Randomization();
        virtual ~Randomization();
        Randomization(const Randomization& other) = delete;
        Randomization& operator=(const Randomization& other) = delete;
    };
    
    /**
     * Creates a "simple" Randomization object, with a 50% chance of assigning variables
     * to "true".
     */
    std::unique_ptr<Randomization> createSimpleRandomization();
}

#endif
