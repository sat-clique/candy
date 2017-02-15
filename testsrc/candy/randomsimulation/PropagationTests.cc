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

#include <gtest/gtest.h>

#include <vector>
#include <set>
#include <unordered_map>
#include <algorithm>

#include <randomsimulation/ClauseOrder.h>
#include <randomsimulation/Propagation.h>
#include <randomsimulation/SimulationVector.h>
#include <core/SolverTypes.h>

namespace Candy {
    class MockClauseOrder : public ClauseOrder {
    public:
        void readGates(GateAnalyzer& analyzer) override;
        const std::vector<Glucose::Var> &getInputVariables() const override;
        const std::vector<Glucose::Lit> &getGateOutputsOrdered() const override;
        const std::vector<const Cl*> &getClauses(Glucose::Var variable) const override;
        void setGateFilter(std::unique_ptr<GateFilter> gateFilter) override;
        unsigned int getAmountOfVars() const override;
        
        void addAnd(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit o, bool pos = true);
        void addAnd(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit i3, Glucose::Lit o, bool pos = true);
        void addOr(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit o, bool pos = true);
        void addOr(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit i3, Glucose::Lit o, bool pos = true);
        
        MockClauseOrder();
        virtual ~MockClauseOrder();
        MockClauseOrder(const ClauseOrder& other) = delete;
        MockClauseOrder& operator=(const MockClauseOrder& other) = delete;
        
    private:
        void addGateWithInput(Glucose::Lit i);
        void addGateWithOutput(Glucose::Lit o);
        
        Glucose::Var m_maxVar = -1;
        std::vector<Glucose::Var> m_inputVariables_v {};
        std::set<Glucose::Var> m_outputVariables {};
        std::set<Glucose::Var> m_inputVariables {};
        std::vector<Glucose::Lit> m_outputs {};
        std::unordered_map<Glucose::Var, std::vector<const Cl*>> m_gateClauses {};
    };
    
    MockClauseOrder::MockClauseOrder() {
    }
    
    MockClauseOrder::~MockClauseOrder() {
        for (auto kv : m_gateClauses) {
            for (auto clause : kv.second) {
                delete clause;
            }
        }
    }
    
    void MockClauseOrder::readGates(GateAnalyzer &) {
    }
    
    void MockClauseOrder::setGateFilter(std::unique_ptr<GateFilter>) {
        // no implementation
    }
    
    const std::vector<Glucose::Var> &MockClauseOrder::getInputVariables() const {
        return m_inputVariables_v;
    }
    
    const std::vector<Glucose::Lit> &MockClauseOrder::getGateOutputsOrdered() const {
        return m_outputs;
    }
    
    const std::vector<const Cl*> &MockClauseOrder::getClauses(Glucose::Var variable) const {
        auto resultIter = m_gateClauses.find(variable);
        assert (resultIter != m_gateClauses.end());
        return resultIter->second;
    }
    
    unsigned int MockClauseOrder::getAmountOfVars() const {
        return m_maxVar + 1;
    }
    
    void MockClauseOrder::addGateWithInput(Glucose::Lit i) {
        if (m_outputVariables.find(Glucose::var(i)) != m_outputVariables.end()) {
            return;
        }
        if (m_inputVariables.find(Glucose::var(i)) == m_inputVariables.end()) {
            m_inputVariables_v.push_back(Glucose::var(i));
            m_inputVariables.insert(Glucose::var(i));
        }
        
        m_maxVar = std::max(m_maxVar, Glucose::var(i));
    }
    
    void MockClauseOrder::addGateWithOutput(Glucose::Lit o) {
        assert(m_inputVariables.find(Glucose::var(o)) == m_inputVariables.end());
        assert(m_outputVariables.find(Glucose::var(o)) == m_outputVariables.end());
        m_outputVariables.insert(Glucose::var(o));
        m_outputs.push_back(o);
        
        m_maxVar = std::max(m_maxVar, Glucose::var(o));
    }
    
    
    void MockClauseOrder::addAnd(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit o, bool pos) {
        addGateWithInput(i1);
        addGateWithInput(i2);
        
        if (pos) {
            addGateWithOutput(o);
            Cl* cl1 = new Cl();
            cl1->push_back(~i1);
            cl1->push_back(~i2);
            cl1->push_back(o);
            m_gateClauses[Glucose::var(o)].push_back(cl1);
        }
        else {
            addGateWithOutput(~o);
            Cl* cl2 = new Cl();
            cl2->push_back(i1);
            cl2->push_back(~o);
            
            Cl* cl3 = new Cl();
            cl3->push_back(i2);
            cl3->push_back(~o);
            
            m_gateClauses[Glucose::var(o)].push_back(cl2);
            m_gateClauses[Glucose::var(o)].push_back(cl3);
        }
    }
    
    void MockClauseOrder::addAnd(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit i3, Glucose::Lit o, bool pos) {
        addGateWithInput(i1);
        addGateWithInput(i2);
        addGateWithInput(i3);
        
        if (pos) {
            addGateWithOutput(o);
            Cl* cl1 = new Cl();
            cl1->push_back(~i1);
            cl1->push_back(~i2);
            cl1->push_back(~i3);
            cl1->push_back(o);
            m_gateClauses[Glucose::var(o)].push_back(cl1);
        }
        else {
            addGateWithOutput(~o);
            Cl* cl2 = new Cl();
            cl2->push_back(i1);
            cl2->push_back(~o);
            
            Cl* cl3 = new Cl();
            cl3->push_back(i2);
            cl3->push_back(~o);
            
            Cl* cl4 = new Cl();
            cl4->push_back(i3);
            cl4->push_back(~o);
            m_gateClauses[Glucose::var(o)].push_back(cl2);
            m_gateClauses[Glucose::var(o)].push_back(cl3);
            m_gateClauses[Glucose::var(o)].push_back(cl4);
        }
    }
    
    void MockClauseOrder::addOr(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit o, bool pos) {
        addGateWithInput(i1);
        addGateWithInput(i2);
        
        if (!pos) {
            addGateWithOutput(~o);
            Cl* cl1 = new Cl();
            cl1->push_back(i1);
            cl1->push_back(i2);
            cl1->push_back(~o);
            m_gateClauses[Glucose::var(o)].push_back(cl1);
        }
        else {
            addGateWithOutput(o);
            Cl* cl2 = new Cl();
            cl2->push_back(~i1);
            cl2->push_back(o);
            
            Cl* cl3 = new Cl();
            cl3->push_back(~i2);
            cl3->push_back(o);
            
            
            m_gateClauses[Glucose::var(o)].push_back(cl2);
            m_gateClauses[Glucose::var(o)].push_back(cl3);
        }
    }
    
    void MockClauseOrder::addOr(Glucose::Lit i1, Glucose::Lit i2, Glucose::Lit i3, Glucose::Lit o, bool pos) {
        addGateWithInput(i1);
        addGateWithInput(i2);
        addGateWithInput(i3);
        
        if (!pos) {
            addGateWithOutput(~o);
            Cl* cl1 = new Cl();
            cl1->push_back(i1);
            cl1->push_back(i2);
            cl1->push_back(i3);
            cl1->push_back(~o);
            m_gateClauses[Glucose::var(o)].push_back(cl1);
        }
        else {
            addGateWithOutput(o);
            Cl* cl2 = new Cl();
            cl2->push_back(~i1);
            cl2->push_back(o);
            
            Cl* cl3 = new Cl();
            cl3->push_back(~i2);
            cl3->push_back(o);
            
            Cl* cl4 = new Cl();
            cl4->push_back(~i3);
            cl4->push_back(o);
            
            m_gateClauses[Glucose::var(o)].push_back(cl2);
            m_gateClauses[Glucose::var(o)].push_back(cl3);
            m_gateClauses[Glucose::var(o)].push_back(cl4);
        }
        
    }
    
    TEST (RSPropagationTest, noClauses) {
        MockClauseOrder mco;
        auto underTest = createInputToOutputPropagation();
        SimulationVectors assignment;
        assignment.initialize(2);
        
        assignment.get(0).vars[0] = 0xDEADull;
        assignment.get(1).vars[0] = 0xBEEFull;
        
        underTest->propagate(assignment, mco);
        
        EXPECT_EQ(assignment.get(0).vars[0], 0xDEADull);
        EXPECT_EQ(assignment.get(1).vars[0], 0xBEEFull);
    }
    
    TEST (RSPropagationTest, singleGatePropagation) {
        MockClauseOrder mco;
        auto underTest = createInputToOutputPropagation();
        SimulationVectors assignment;
        
        assignment.initialize(3);
        
        mco.addAnd(Glucose::mkLit(2, 1), Glucose::mkLit(0, 1), Glucose::mkLit(1, 1), false);
        
        assignment.get(0).vars[0] = 0x5F00ull;
        assignment.get(2).vars[0] = 0x9F00ull;
        assignment.get(1).vars[0] = 0xDEADBEEFull;
        
        underTest->propagate(assignment, mco);
        
        EXPECT_EQ(assignment.get(0).vars[0], 0x5F00ull);
        EXPECT_EQ(assignment.get(2).vars[0], 0x9F00ull);
        EXPECT_EQ(assignment.get(1).vars[0], 0x1F00ull);
    }
    
    TEST (RSPropagationTest, multiGatePropagation) {
        MockClauseOrder mco;
        auto underTest = createInputToOutputPropagation();
        SimulationVectors assignment;
        
        assignment.initialize(7);
        
        mco.addAnd(Glucose::mkLit(2, 1), Glucose::mkLit(0, 1), Glucose::mkLit(1, 1), false);
        mco.addOr(Glucose::mkLit(1, 1), Glucose::mkLit(3, 0), Glucose::mkLit(4, 0), Glucose::mkLit(5, 1), true);
        mco.addAnd(Glucose::mkLit(5, 1), Glucose::mkLit(1, 0), Glucose::mkLit(6, 0), true);
        
        assignment.get(0).vars[2] =  0x5F00ull;
        assignment.get(2).vars[2] =  0x9F00ull;
        assignment.get(1).vars[2] =  0xDEADBEEFull;
        assignment.get(3).vars[2] = ~0x0001ull;
        assignment.get(4).vars[2] = ~0x0020ull;
        assignment.get(5).vars[2] =  0x0DEADBEEull;
        assignment.get(6).vars[2] =  0x0DEADBECull;

        
        underTest->propagate(assignment, mco);
        
        EXPECT_EQ(assignment.get(0).vars[2],  0x5F00ull);
        EXPECT_EQ(assignment.get(2).vars[2],  0x9F00ull);
        EXPECT_EQ(assignment.get(1).vars[2],  0x1F00ull);
        EXPECT_EQ(assignment.get(3).vars[2], ~0x0001ull);
        EXPECT_EQ(assignment.get(4).vars[2], ~0x0020ull);
        EXPECT_EQ(assignment.get(5).vars[2],  0x1F21ull);
        EXPECT_EQ(assignment.get(6).vars[2], ~0x0021ull);
    }
}
