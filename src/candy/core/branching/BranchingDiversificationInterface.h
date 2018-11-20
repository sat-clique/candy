#ifndef BRANCHING_DIVERSIFICATION_INTERFACE_H_
#define BRANCHING_DIVERSIFICATION_INTERFACE_H_

#include "candy/core/SolverTypes.h"

namespace Candy {

class BranchingDiversificationInterface {

    virtual void setPolarity(Var v, bool sign) = 0;

    virtual Lit getLastDecision() = 0;

};

}

#endif