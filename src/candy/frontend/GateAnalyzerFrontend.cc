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

#include "GateAnalyzerFrontend.h"

#include <candy/utils/MemUtils.h>
#include <candy/gates/GateAnalyzer.h>

namespace Candy {
    
    std::ostream& operator <<(std::ostream& stream, const GateRecognitionArguments& arguments) {
        stream << "c Gate recognition arguments: " << std::endl
        << "c   Max tries: " << arguments.opt_gr_tries << std::endl
        << "c   Patterns: " << arguments.opt_gr_patterns << std::endl
        << "c   Semantic: " << arguments.opt_gr_semantic << std::endl
        << "c   Semantic budget: " << arguments.opt_gr_semantic_budget << std::endl
        << "c   Timeout: " << arguments.opt_gr_timeout.count() << " ms" << std::endl
        << "c   Holistic: " << arguments.opt_gr_holistic << std::endl
        << "c   Lookahead: " << arguments.opt_gr_lookahead << std::endl
        << "c   Intensify: " << arguments.opt_gr_intensify << std::endl
        << "c   Lookahead threshold: " << arguments.opt_gr_lookahead_threshold << std::endl
        << "c   Print gates: " << arguments.opt_print_gates << std::endl;
        return stream;
    }
    
    std::unique_ptr<GateAnalyzer> createGateAnalyzer(CNFProblem &problem,
                                                     const GateRecognitionArguments recognitionArgs) {
        return backported_std::make_unique<Candy::GateAnalyzer>(problem,
                                                                recognitionArgs.opt_gr_tries,
                                                                recognitionArgs.opt_gr_patterns,
                                                                recognitionArgs.opt_gr_semantic,
                                                                recognitionArgs.opt_gr_holistic,
                                                                recognitionArgs.opt_gr_lookahead,
                                                                recognitionArgs.opt_gr_intensify,
                                                                recognitionArgs.opt_gr_lookahead_threshold,
                                                                recognitionArgs.opt_gr_semantic_budget,
                                                                recognitionArgs.opt_gr_timeout);
    }
    

    void benchmarkGateRecognition(Candy::CNFProblem &problem, const GateRecognitionArguments& recognitionArgs) {
        Statistics::getInstance().runtimeStart("Gate analysis");
        auto gates = createGateAnalyzer(problem, recognitionArgs);
        gates->analyze();
        //Statistics::getInstance().runtimeStop("Gate analysis");
        std::cout << "gates:" << gates->getGateCount() << std::endl;
        std::cout << "variables:" << problem.nVars() << std::endl;
        std::cout << "clauses:" << problem.nClauses() << std::endl;
        std::cout << "analyzer-runtime:" << gates->runtime.getRuntime().count() / 1000 << std::endl;
        //Statistics::getInstance().printRuntime("Gate analysis");
        if (recognitionArgs.opt_print_gates) {
            gates->printGates();
        }
    }

}
