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
    std::vector<lbool> minimizedModel;

    // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.
    std::vector<Lit> conflict; 

public:
    CandySolverResult() : status(l_Undef), model(), conflict() { }

    CandySolverResult(std::initializer_list<Lit> model) : status(l_Undef), model(), conflict() { 
        for (Lit lit : model) {
            setModelValue(lit);
        }
    }

    void clear() {
        status = l_Undef;
        model.clear();
        conflict.clear();
    }

    void setStatus(lbool status_) {
        status = status_;
    }
    
    void setModel(Trail& trail) {
        for (Lit lit : trail) {
            setModelValue(lit);
        }
    }

    void setModelValue(Lit lit) {
        if (lit.var() >= (Var)model.size()) {
            model.resize(lit.var()+1, l_Undef);
        }
        model[lit.var()] = lit.sign() ? l_False : l_True;
    }

    void setConflict(std::vector<Lit> assumptions) {
        conflict.insert(conflict.end(), assumptions.begin(), assumptions.end());
    }

    bool satisfies(Lit lit) {
        return (Var)model.size() > lit.var() && l_True == (model[lit.var()] ^ lit.sign());
    }

    // return satisfied literal for given variable
    Lit value(Var x) const {
        if (model[x] == l_False) {
            return Lit(x, true);
        } 
        else if (model[x] == l_True) {
            return Lit(x, false);
        }
        else {
            return lit_Undef;
        }
    }

    std::vector<Lit> getModelLiterals() {
        std::vector<Lit> literals; 
        for (Var v = 0; v < (Var)model.size(); v++) {
            if (model[v] == l_True) {
                literals.push_back(Lit(v, false));
            }
            else if (model[v] == l_False) {
                literals.push_back(Lit(v, true));
            }
        }
        return literals;
    }

    std::vector<Lit>& getConflict() {
        return conflict;
    }

    /** support for minimized models */
    void setMinimizedModelValue(Lit lit) {
        if (lit.var() >= (Var)minimizedModel.size()) {
            minimizedModel.resize(lit.var()+1, l_Undef);
        }
        minimizedModel[lit.var()] = lit.sign() ? l_False : l_True;
    }

    std::vector<Lit> getMinimizedModelLiterals() {
        std::vector<Lit> literals; 
        for (Var v = 0; v < (Var)minimizedModel.size(); v++) {
            if (minimizedModel[v] == l_True) {
                literals.push_back(Lit(v, false));
            }
            else if (minimizedModel[v] == l_False) {
                literals.push_back(Lit(v, true));
            }
        }
        return literals;
    }

};
}

#endif