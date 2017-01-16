#ifndef _15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H
#define _15FB8CE7_2B9D_4A41_9F1E_BCD17A4E76E0_CLAUSEORDER_H

#include <vector>
#include <memory>

#include <core/SolverTypes.h>

class GateAnalyzer;

namespace randsim {
    
    class ClauseOrder {
    public:
        virtual void readGates(GateAnalyzer& analyzer) = 0;
        virtual const std::vector<Glucose::Var> &getInputVariables() const = 0;
        virtual const std::vector<Glucose::Lit> &getGateOutputsOrdered() const = 0;
        virtual const std::vector<const Glucose::Cl*> &getClauses(Glucose::Var variable) const = 0;
        virtual unsigned int getAmountOfVars() const = 0;
        
        ClauseOrder();
        virtual ~ClauseOrder();
        ClauseOrder(const ClauseOrder& other) = delete;
        ClauseOrder& operator=(const ClauseOrder& other) = delete;
    };
    
    std::unique_ptr<ClauseOrder> createDefaultClauseOrder();
}

#endif
