#include "RandomSimulationFrontend.h"

#include <candy/randomsimulation/RandomSimulator.h>
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

#include <candy/randomsimulation/Conjectures.h>
#include <candy/randomsimulation/ClauseOrder.h>
#include <candy/randomsimulation/SimulationVector.h>

#include <candy/utils/MemUtils.h>

#include <iostream>

namespace Candy {
    std::unique_ptr<Candy::Conjectures> performRandomSimulation(const Candy::GateAnalyzer &analyzer,
                                                                const RandomSimulationArguments& rsArguments,
                                                                std::chrono::milliseconds timeLimit) {
        auto simulatorBuilder = Candy::createDefaultRandomSimulatorBuilder();
        simulatorBuilder->withGateAnalyzer(analyzer);
        
        if (rsArguments.abortByRRAT) {
            simulatorBuilder->withReductionRateAbortThreshold(rsArguments.rrat);
        }
        
        if (rsArguments.filterGatesByNonmono) {
            simulatorBuilder->withGateFilter(Candy::createNonmonotonousGateFilter(analyzer));
        }
        
        auto randomSimulator = simulatorBuilder->build();
        // TODO: time limit
        auto conjectures = randomSimulator->run(static_cast<unsigned int>(rsArguments.nRounds), timeLimit);
        
        if (rsArguments.filterConjecturesBySize) {
            auto sizeFilter = Candy::createSizeConjectureFilter(rsArguments.maxConjectureSize);
            conjectures = sizeFilter->apply(conjectures);
        }
        
        if (rsArguments.removeBackboneConjectures) {
            auto bbFilter = Candy::createBackboneRemovalConjectureFilter();
            conjectures = bbFilter->apply(conjectures);
        }
        
        return backported_std::make_unique<Candy::Conjectures>(std::move(conjectures));
    }
    
    std::ostream& operator <<(std::ostream& stream, const RandomSimulationArguments& arguments) {
        stream << "c Random simulation arguments: " << std::endl
        << "c   Rounds: " << arguments.nRounds << std::endl
        << "c   RRAT abort heuristic enabled: " << arguments.abortByRRAT << std::endl
        << "c   RRAT: " << arguments.rrat << std::endl
        << "c   Filtering by conjecture size enabled: " << arguments.filterConjecturesBySize << std::endl
        << "c   Max. conjecture size: " << arguments.maxConjectureSize << std::endl
        << "c   Remove backbone conjectures: " << arguments.removeBackboneConjectures << std::endl
        << "c   Remove conjectures about monotonously nested gates: " << arguments.filterGatesByNonmono << std::endl
        << "c   Preprocessing time limit: " << arguments.preprocessingTimeLimit.count() << " ms" << std::endl;
        
        return stream;
    }

}
