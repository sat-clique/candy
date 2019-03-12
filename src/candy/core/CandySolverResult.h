/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef CANDY_SOLVER_RESULT
#define CANDY_SOLVER_RESULT

#include <vector>

#include "candy/core/SolverTypes.h"
#include "candy/core/Trail.h"

namespace Candy {

class CandySolverResult {
    lbool status;

     // If problem is satisfiable, this vector contains the model
    std::vector<lbool> model;

    // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.
    std::vector<Lit> conflict; 

public:
    CandySolverResult() : status(l_Undef), model(), conflict() { }

    void clear() {
        status = l_Undef;
        model.clear();
        conflict.clear();
    }

    void setStatus(lbool status_) {
        status = status_;
    }
    
    void setModel(Trail& trail) {
        model.resize(trail.trail.size(), l_Undef);
        for (Lit lit : trail) {
            model[lit.var()] = lit.sign() ? l_False : l_True;
        }
    }

    void setConflict(std::vector<Lit> assumptions) {
        conflict.insert(conflict.end(), assumptions.begin(), assumptions.end());
    }

    lbool modelValue(Var x) const {
        return model[x];
    }

    lbool modelValue(Lit p) const {
        return model[p.var()] ^ p.sign();
    }

    std::vector<lbool>& getModelValues() {
        return model;
    }

    std::vector<Lit> getModelLiterals() {
        std::vector<Lit> model_literals; 
        Var v = 0;
        for (lbool value : model) {
            if (value == l_True) {
                model_literals.push_back(Lit(v, false));
            }
            else if (value == l_False) {
                model_literals.push_back(Lit(v, true));
            }         
            v++;
        }
        return model_literals;
    }

    std::vector<Lit>& getConflict() {
        return conflict;
    }

};
}

#endif