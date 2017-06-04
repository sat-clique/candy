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
    struct RandomSimulationArguments {
        const int nRounds;
        const bool abortByRRAT;
        const double rrat;
        const bool filterConjecturesBySize;
        const int maxConjectureSize;
        const bool removeBackboneConjectures;
        const bool filterGatesByNonmono;
        std::chrono::milliseconds preprocessingTimeLimit;
    };
    
    std::ostream& operator <<(std::ostream& stream, const RandomSimulationArguments& arguments);
    
    std::unique_ptr<Candy::Conjectures> performRandomSimulation(Candy::GateAnalyzer &analyzer,
                                                                const RandomSimulationArguments& rsArguments,
                                                                std::chrono::milliseconds timeLimit = std::chrono::milliseconds{-1});
}

#endif
