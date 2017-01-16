#ifndef _15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H
#define _15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H

#include <vector>
#include <memory>

#include <core/SolverTypes.h>

class GateAnalyzer;

namespace randsim {
    class Clause;
    
    class ClauseOrder {
    public:
        
        virtual void readGates(GateAnalyzer& analyzer) = 0;
        virtual std::vector<Glucose::Var> &getInputVariables() = 0;
        virtual std::vector<Glucose::Lit> &getGateOutputsOrdered() = 0;
        virtual std::vector<Glucose::Cl> &getClauses(Glucose::Var variable) = 0;
        virtual unsigned int getAmountOfVars() = 0;
        
        ClauseOrder();
        virtual ~ClauseOrder();
        ClauseOrder(const ClauseOrder& other) = delete;
        ClauseOrder& operator=(const ClauseOrder& other) = delete;
    };
    
    std::unique_ptr<ClauseOrder> createDefaultClauseOrder();
}

#endif
