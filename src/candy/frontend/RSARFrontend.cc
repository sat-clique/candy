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

#include "RSARFrontend.h"

#include <stdexcept>

#include <candy/utils/StringUtils.h>
#include <candy/rsar/ARSolver.h>
#include <candy/rsar/Heuristics.h>
#include <candy/randomsimulation/Conjectures.h>

namespace Candy {
    
    std::ostream& operator <<(std::ostream& stream, const RSARArguments& arguments) {
        stream << "c RSAR arguments: " << std::endl << "c   RSAR enabled: " << arguments.useRSAR << std::endl;
        if (arguments.useRSAR) {
            stream << "c   Max. refinement steps: " << arguments.maxRefinementSteps << std::endl
            << "c   Simplification handling mode: " << static_cast<int>(arguments.simplificationHandlingMode) << std::endl
            << "c   Use input dependency size heuristic: " << arguments.withInputDepCountHeuristic << std::endl
            << "c   Input dependency size heuristic configuration: " << arguments.inputDepCountHeuristicConfiguration << std::endl
            << "c   Min. gate count: " << arguments.minGateCount << std::endl;
        }
        
        return stream;
    }
    
    SimplificationHandlingMode parseSimplificationHandlingMode(const std::string& str) {
        if (str == "DISABLE") {
            return Candy::SimplificationHandlingMode::DISABLE;
        }
        if (str == "FREEZE") {
            return Candy::SimplificationHandlingMode::FREEZE;
        }
        if (str == "RESTRICT") {
            return Candy::SimplificationHandlingMode::RESTRICT;
        }
        if (str == "FULL") {
            return Candy::SimplificationHandlingMode::FULL;
        }
        throw std::invalid_argument(str + ": Unknown simplification handling mode");
    }
    
}
