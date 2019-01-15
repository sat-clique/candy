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
#include <candy/randomsimulation/RandomSimulator.h>
#include <candy/randomsimulation/Conjectures.h>
#include <candy/gates/GateAnalyzer.h>
#include <candy/utils/MemUtils.h>

namespace Candy {

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
    static std::unique_ptr<Conjectures> performRandomSimulation(const GateAnalyzer& analyzer, std::chrono::milliseconds timeLimit) {
        BitparallelRandomSimulatorBuilder simulatorBuilder { analyzer };

        auto randomSimulator = simulatorBuilder.build();

        auto conjectures = randomSimulator->run(static_cast<unsigned int>(RandomSimulationOptions::opt_rs_nrounds), timeLimit);
        
        if (RandomSimulationOptions::opt_rs_filterConjBySize > 0) {
            auto sizeFilter = Candy::createSizeConjectureFilter(RandomSimulationOptions::opt_rs_filterConjBySize);
            conjectures = sizeFilter->apply(conjectures);
        }
        
        if (RandomSimulationOptions::opt_rs_removeBackboneConj) {
            auto bbFilter = Candy::createBackboneRemovalConjectureFilter();
            conjectures = bbFilter->apply(conjectures);
        }
        
        return backported_std::make_unique<Conjectures>(std::move(conjectures));
    }
}

#endif
