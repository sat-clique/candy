/* Copyright (c) 2017 Markus Iser, Felix Kutzner (github.com/fkutzner)
 
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

#ifndef X_C5BE85EF_50D8_4973_BA53_22FCD996004D_H
#define X_C5BE85EF_50D8_4973_BA53_22FCD996004D_H

#include <chrono>
#include <iostream>
#include <memory>

#include <candy/core/CNFProblem.h>

namespace Candy {
    class GateAnalyzer;
    
    struct GateRecognitionArguments {
        int opt_gr_tries;
        bool opt_gr_patterns;
        bool opt_gr_semantic;
        unsigned int opt_gr_semantic_budget;
        std::chrono::milliseconds opt_gr_timeout;
        bool opt_gr_holistic;
        bool opt_gr_lookahead;
        bool opt_gr_intensify;
        int opt_gr_lookahead_threshold;
        bool opt_print_gates;
    };
    
    std::ostream& operator <<(std::ostream& stream, const GateRecognitionArguments& arguments);
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Creates a GateAnalyzer instance for the given CNF problem and gate analysis arguments.
     *
     * \param problem           the CNF problem.
     * \param recognitionArgs   the gate recognition arguments.
     *
     * \returns a unique_ptr to the newly created gate analyzer (transferring ownership of the gate
     *   analyzer to the caller.)
     */
    std::unique_ptr<GateAnalyzer> createGateAnalyzer(CNFProblem &problem,
                                                     const GateRecognitionArguments recognitionArgs);
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Performs gate recognition on the problem \p dimacs and prints statistics.
     *
     * TODO: document parameters
     */
    void benchmarkGateRecognition(Candy::CNFProblem &problem, const GateRecognitionArguments& recognitionArgs);
}
#endif
