#ifndef _15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H
#define _15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H

#include <vector>
#include <memory>

class GateAnalyzer;

namespace randsim {
    typedef size_t Var;
    class Clause;
    
    class ClauseOrder {
    public:
        
        virtual void readGates(GateAnalyzer& analyzer) = 0;
        virtual std::vector<Var> &getInputVariables() = 0;
        virtual std::vector<Var> &getGateOutputsOrdered() = 0;
        virtual std::vector<Clause> &getClauses(Var variable) = 0;
        virtual unsigned int getAmountOfVars() = 0;
        
        ClauseOrder();
        virtual ~ClauseOrder();
        ClauseOrder(const ClauseOrder& other) = delete;
        ClauseOrder& operator=(const ClauseOrder& other) = delete;
    };
    
    std::unique_ptr<ClauseOrder> createDefaultClauseOrder();
}

#endif
