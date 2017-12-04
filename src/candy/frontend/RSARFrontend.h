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

#include <candy/rsar/ARSolver.h>

namespace Candy {
    class GateAnalyzer;
    class DefaultSimpSolver;
    class Conjectures;
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief RSAR argument structure
     */
    struct RSARArguments {
        /// iff true, RSAR should be used for SAT solving.
        const bool useRSAR;
        
        /// the maximum amount of refinements computed after the initial one. If this budget is depleted, the original problem is solved.
        const int maxRefinementSteps;
        
        /// the simplification handling mode to be used for RSAR initialization.
        const Candy::SimplificationHandlingMode simplificationHandlingMode;
        
        /// iff true, the input-dependency-count refinement heuristic is used.
        const bool withInputDepCountHeuristic;
        
        /// the configuration for the input-dependency-count refinement heuristic.
        const std::string inputDepCountHeuristicConfiguration;
        
        /// the minimum amount of gates needed to be recognized in a problem for RSAR to be used.
        const int minGateCount;
    };
    
    std::ostream& operator <<(std::ostream& stream, const RSARArguments& arguments);
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Parses RSAR simplification handling mode strings.
     *
     * Throws std::invalid_argument if the simplification handling mode string is illegal.
     *
     * \param str   the string representing the simplification mode. Must be one of
     *              FREEZE, RESTRICT, FULL, DISABLE.
     *
     * \returns the SimplificationHandlingMode value corresponding to the parameter str.
     */
    SimplificationHandlingMode parseSimplificationHandlingMode(const std::string& str);
    
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Creates an ARSolver instance.
     *
     * \param analyzer      a GateAnalyzer using which the CNF problem to be solved has been analyzed.
     * \param satSolver     the SAT solver which should be used by RSAR.
     * \param conjectures   equivalence/backbone conjectures which should be used to compute abstractions.
     * \param rsarArguments the RSAR configuration.
     *
     * \returns a unique_pointer to the new ARSolver instance (transferring ownership of the ARSolver
     *   instance to the caller).
     */
    CandySolverInterface* createARSolver(const GateAnalyzer& analyzer,
                                                    CandySolverInterface* satSolver,
                                                    std::unique_ptr<Conjectures> conjectures,
                                                    const RSARArguments& rsarArguments);
}

#endif
