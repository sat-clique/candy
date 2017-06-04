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

#ifndef X_C68DAB8D_F573_4EEB_90C7_B7B0CC7795B9_H
#define X_C68DAB8D_F573_4EEB_90C7_B7B0CC7795B9_H

#include <rsar/ARSolver.h>

namespace Candy {
    class GateAnalyzer;
    class DefaultSimpSolver;
    class Conjectures;
    
    struct RSARArguments {
        const bool useRSAR;
        const int maxRefinementSteps;
        const Candy::SimplificationHandlingMode simplificationHandlingMode;
        const bool withInputDepCountHeuristic;
        const std::string inputDepCountHeuristicConfiguration;
        const int minGateCount;
    };
    
    std::ostream& operator <<(std::ostream& stream, const RSARArguments& arguments);
    
    Candy::SimplificationHandlingMode parseSimplificationHandlingMode(const std::string& str);
    
    
    std::vector<size_t> getARInputDepCountHeuristicLimits(const std::string& limitsString);
    
    std::unique_ptr<Candy::ARSolver> createARSolver(GateAnalyzer& analyzer,
                                                    DefaultSimpSolver& satSolver,
                                                    std::unique_ptr<Conjectures> conjectures,
                                                    const RSARArguments& rsarArguments);
}

#endif
