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

#ifndef X_1CF42BF1_32A9_4A8D_ADC3_D2FA7FAC4AC8_H
#define X_1CF42BF1_32A9_4A8D_ADC3_D2FA7FAC4AC8_H

#include <memory>
#include <chrono>
#include <candy/randomsimulation/Conjectures.h>
#include <candy/gates/GateAnalyzer.h>

namespace Candy {
    /**
     * \ingroup CandyFrontend
     *
     * \brief Random simulation argument structure
     */
    struct RandomSimulationArguments {
        /// The maximum amount of random simulation steps to be performed. This amount may be rounded up to the nearest multiple of 2048.
        const int nRounds;
        
        /// Iff true, random simulation may be aborted due to the reduction rate abort threshold being too low.
        const bool abortByRRAT;
        
        /// The reduction rate abort threshold.
        const double rrat;
        
        /// If true, conjectures larger than maxConjectureSize are discarded.
        const bool filterConjecturesBySize;
        
        /// The maximum conjecture size (ignored if filterConjecturesBySize == false).
        const int maxConjectureSize;
        
        /// Remove backbone variables from the conjectures produced by random simulation.
        const bool removeBackboneConjectures;
        
        /// If true, only nonmonotonously nested gates are taken into account for random simulation.
        const bool filterGatesByNonmono;
        
        /// The random simulation time limit. If negative, no time limit is used.
        std::chrono::milliseconds preprocessingTimeLimit;
    };
    
    std::ostream& operator <<(std::ostream& stream, const RandomSimulationArguments& arguments);
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Performs random simulation on the given gate structure, throwing an OutOfTimeException
     *   if a time limit is given and exceeded.
     *
     * \param analyzer      the target gate structure.
     * \param rsArguments   the random simulation arguments.
     * \param timeLimit     the time limit. If negative, time limiting is disabled, which is the default.
     *
     * \returns a set of conjectures about equivalences and backbone variables in the given gate structure
     *   (transferring ownership of the returned object to the caller).
     */
    std::unique_ptr<Candy::Conjectures> performRandomSimulation(Candy::GateAnalyzer &analyzer,
                                                                const RandomSimulationArguments& rsArguments,
                                                                std::chrono::milliseconds timeLimit = std::chrono::milliseconds{-1});
}

#endif
