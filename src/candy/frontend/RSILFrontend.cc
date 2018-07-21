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

#include "RSILFrontend.h"

namespace Candy {
    std::ostream& operator <<(std::ostream& stream, const RSILArguments& arguments) {
        stream << "c RSIL arguments: " << std::endl << "c   RSIL enabled: " << arguments.useRSIL << std::endl;
        if (arguments.useRSIL) {
            stream << "c   RSIL mode: " << static_cast<int>(arguments.mode) << std::endl
            << "c   Vanishing mode half-life: " << arguments.vanishing_probabilityHalfLife << std::endl
            << "c   Implication budget mode initial budgets: " << arguments.impbudget_initialBudget << std::endl
            << "c   Filter by input dependency count enabled: " << arguments.filterByInputDependencies << std::endl
            << "c   Max. input depdencency count: " << arguments.filterByInputDependenciesMax << std::endl
            << "c   Apply filters only to backbone conjectures: " << arguments.filterOnlyBackbones << std::endl
            << "c   Min. gate output fraction: " << arguments.minGateOutputFraction << std::endl
            << "c   RSIL restricted to miters?: " << arguments.useRSILOnlyForMiters << std::endl;
        }
        
        return stream;
    }
    
    RSILMode getRSILMode(const std::string& mode) {
        if (mode == "unrestricted") {
            return RSILMode::UNRESTRICTED;
        }
        else if (mode == "vanishing") {
            return RSILMode::VANISHING;
        }
        else if (mode == "implicationbudgeted") {
            return RSILMode::IMPLICATIONBUDGETED;
        }
        else {
            throw std::invalid_argument("Error: unknown RSIL mode " + mode);
        }
    }

}
