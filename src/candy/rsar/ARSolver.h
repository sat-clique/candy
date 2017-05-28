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

#ifndef X_27915A76_7E30_4C34_8C0C_FD65894E0A42_ARSOLVER_H
#define X_27915A76_7E30_4C34_8C0C_FD65894E0A42_ARSOLVER_H

#include <candy/core/SolverTypes.h>
#include <candy/utils/CNFProblem.h>

#include <vector>
#include <memory>

namespace Candy {
    class Conjectures;
    class RefinementHeuristic;
    class SolverAdapter;
    
    /**
     * \defgroup RS_AbstractionRefinement
     */
    
    /**
     * \class ARSolver
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A SAT solver performing abstraction refinement.
     *
     * TODO: detailed description
     */
    class ARSolver {
    public:
        /** 
         * Returns true iff the given SAT problem instance is satifsiable.
         */
        virtual lbool solve(CNFProblem &problem) = 0;
        
        /**
         * Returns true iff the SAT problem instance with which the underlying
         * SAT solver has been instantiated is satisfiable.
         */
        virtual lbool solve() = 0;
        
        ARSolver() noexcept;
        virtual ~ARSolver();
        ARSolver(const ARSolver& other) = delete;
        ARSolver& operator=(const ARSolver& other) = delete;
    };
    
    enum class SimplificationHandlingMode {
        DISABLE, /// Disable problem simplification entirely
        RESTRICT, /// Simplify the SAT problem after creating the first approximation
        FREEZE, /// like RESTRICT, but freeze the variables occuring involved in the first approximation
        FULL /// Simplify the problem first, then compute the approximations using the remaining variables
    };
    
    /**
     * \class ARSolverBuilder
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A builder for ARSolver instances.
     */
    class ARSolverBuilder {
    public:
        /**
         * Sets the conjectures (about equivalencies between literals in the SAT problem
         * instance and the problem instance's backbone) which should be used to compute
         * approximations. The default is not to use any conjectures, reducing the
         * solving process to plain SAT solving.
         */
        virtual ARSolverBuilder& withConjectures(std::unique_ptr<Conjectures> conjectures) noexcept = 0;
        
        /**
         * Sets the incremental SAT solver to be used. By default, a default-constructed
         * instance of Glucose::SimpSolver is used.
         * The solver must be in a state such that
         *  - no clauses have been added yet
         *  - it is possible to call solve(...) functions
         */
        virtual ARSolverBuilder& withSolver(std::unique_ptr<SolverAdapter> solver) noexcept = 0;
        
        /**
         * Sets the maximum amount of refinement steps to be performed. A negative maximum
         * causes removes the bound on the maximum amount of refinement steps. By default,
         * no such bound is used.
         */
        virtual ARSolverBuilder& withMaxRefinementSteps(int maxRefinementSteps) noexcept = 0;
        
        /**
         * Adds the given heuristic to the set of refinement heuristics to be used to compute
         * approximations of the SAT problem instance.
         */
        virtual ARSolverBuilder& addRefinementHeuristic(std::unique_ptr<RefinementHeuristic> heuristic) = 0;
        
        /**
         * Sets the simplification handling mode to be used (see the documentation of
         * SimplificationHandlingMode). The default is SimplificationHandlingMode::RESTRICT.
         */
        virtual ARSolverBuilder& withSimplificationHandlingMode(SimplificationHandlingMode mode) noexcept = 0;
        
        /**
         * Builds the ARSolver instance. Can be called only once.
         */
        virtual std::unique_ptr<ARSolver> build() = 0;
        
        ARSolverBuilder() noexcept;
        virtual ~ARSolverBuilder();
        ARSolverBuilder(const ARSolverBuilder& other) = delete;
        ARSolverBuilder& operator= (const ARSolverBuilder& other) = delete;
    };
    
    /** 
     * \ingroup RS_AbstractionRefinement
     *
     * creates an ARSolverBuilder instance.
     */
    std::unique_ptr<ARSolverBuilder> createARSolverBuilder();
}

#endif
