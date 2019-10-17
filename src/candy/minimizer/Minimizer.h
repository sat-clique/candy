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

#ifndef MINIMIZE_H_
#define MINIMIZE_H_

#include <vector>
#include <map>

#include "candy/core/CandySolverInterface.h"
#include "candy/core/CandySolverResult.h"
#include "candy/core/CNFProblem.h"

namespace Candy {

class Minimizer {

private:
    CandySolverInterface* solver;
    CNFProblem& problem;
    CandySolverResult& model;

    /**
     * normalize and purify the given problem (see our model minimization paper)
     * */  
    template<typename Iterator>
    For purify(Iterator begin, Iterator end);

    /**
     * creates a sat-instance that models the set-cover-problem
     * that solves minimization of the given assignment;
     * pure literals that are set to false in the original problem
     * are stripped out before creating the new instance;
     * Iteratively minimize only maxIterations times (0 => until unsat)
     */
    Cl iterativeMinimization(For hittingSetProblem, bool computeMinimalModel);


    void genHalfAdder(CNFProblem& out, Lit a, Lit b, Lit sum, Lit carry) {
        out.readClause({ a, ~b, sum });
        out.readClause({ ~a, b, sum });
        out.readClause({ ~a, ~b, carry });
    }

    void genFullAdder(CNFProblem& out, Lit a, Lit b, Lit c, Lit sum, Lit carry) {
        out.readClause({ a, b, ~c, sum });
        out.readClause({ a, ~b, c, sum });
        out.readClause({ ~a, b, c, sum });
        out.readClause({ ~a, ~b, ~c, sum });
        out.readClause({ ~a, ~b, carry });
        out.readClause({ ~a, ~c, carry });
        out.readClause({ ~b, ~c, carry });
    }

    /**
     * Generate parallel counter for variables in inputs
     * thanks to Carsten Sinz for the initial implementation and his paper on cardniality constraints
     * */
    Cl genParCounter(CNFProblem& out, Cl inputs) {
        Cl outputs;
        
        if (inputs.size() == 0) {
            return outputs;
        }
        else if (inputs.size() == 1) {
          outputs.push_back(inputs.front());
          return outputs;
        }
            
        int mid = inputs.size() / 2;

        Cl a_inputs { inputs.begin(), inputs.begin() + inputs.size() / 2 };
        Cl b_inputs { inputs.begin() + inputs.size() / 2, inputs.end() };
        Lit carry = inputs.back();
        
        Cl a_outputs = genParCounter(out, a_inputs);
        Cl b_outputs = genParCounter(out, b_inputs);

        int m_min = std::min(a_outputs.size(), b_outputs.size());
        for (int i = 0; i < m_min; i++) {
            Lit sum = Lit(out.newVar());
            Lit nextCarry = Lit(out.newVar());
            genFullAdder(out, a_outputs[i], b_outputs[i], carry, sum, nextCarry);
            outputs.push_back(sum);
            carry = nextCarry;
        }

        for (int i = m_min; i < a_outputs.size(); i++) {
            Lit sum = Lit(out.newVar());
            Lit nextCarry = Lit(out.newVar());
            genHalfAdder(out, a_outputs[i], carry, sum, nextCarry);
            outputs.push_back(sum);
            carry = nextCarry;
        }
        
        outputs.push_back(carry);
        
        return outputs;
    }

    /**
     * Generate less-than constraint for the outputs of the parallel counter
     * */
    void genLessThanConstraint(CNFProblem& out, Cl outputs, int k) {
        For lessthan;
        
        // comparator circuit
        for (int i = 0; i < outputs.size(); i++) {
            if ((k & 1) == 1) {
                for (Cl* clause : lessthan) {
                    clause->push_back(~outputs[i]);
                }
            } else {
                Cl* clause = new Cl();
                clause->push_back(~outputs[i]);
                lessthan.push_back(clause);
            }
            k >>= 1;
        }

        out.readClauses(lessthan);
    }

public:
    Minimizer(CNFProblem& _problem, CandySolverResult& _model);
    virtual ~Minimizer();

    void mimimizeModel(bool pruningActivated, bool computeMinimalModel);

};

}

#endif /* MINIMIZE_H_ */
