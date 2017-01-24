/* Copyright (c) 2017 Felix Kutzner
 
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

#include "TestGateStructure.h"
#include "TestUtils.h"

#include <unordered_set>
#include <cassert>

#include <gtest/gtest.h>

namespace Candy {
    
    GateStructureBuilder::GateStructureBuilder() {
        
    }
    
    GateStructureBuilder::~GateStructureBuilder() {
        
    }
    
    class GateStructureBuilderImpl : public GateStructureBuilder {
    public:
        GateStructureBuilderImpl& withAnd(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) override;
        GateStructureBuilderImpl& withOr(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) override;
        GateStructureBuilderImpl& withXor(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) override;
        std::unique_ptr<CNFProblem, void(*)(CNFProblem*)> build() override;
        
        GateStructureBuilderImpl();
        virtual ~GateStructureBuilderImpl();
        GateStructureBuilderImpl(const GateStructureBuilderImpl& other) = delete;
        GateStructureBuilderImpl& operator=(const GateStructureBuilderImpl& other) = delete;
        
    private:
        std::unique_ptr<CNFProblem> m_builtClauses;
        std::unordered_set<Glucose::Var> m_usedOutputs;
        std::unordered_set<Glucose::Var> m_gateVars;
        
        void addClause(Cl it);
    };
    
    void GateStructureBuilderImpl::addClause(Cl it) {
        m_builtClauses->readClause(it);
    }
    
    GateStructureBuilderImpl::GateStructureBuilderImpl() : GateStructureBuilder(), m_builtClauses(new CNFProblem()),
    m_usedOutputs({}), m_gateVars({}) {
        addClause({Glucose::mkLit(0,1)});
        m_gateVars.insert(0);
    }
    
    GateStructureBuilderImpl::~GateStructureBuilderImpl() {
        
    }
    
    GateStructureBuilderImpl& GateStructureBuilderImpl::withAnd(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) {
        assertContainsVariable(m_gateVars, Glucose::var(output));
        assertDoesNotContainVariable(m_usedOutputs, Glucose::var(output));
        
        Cl fwd = negatedLits(inputs);
        fwd.push_back(output);
        addClause(fwd);
        
        for (auto input : inputs) {
            addClause({input, ~output});
        }
        
        insertVariables(inputs, m_gateVars);
        m_usedOutputs.insert(Glucose::var(output));
        return *this;
    }
    
    GateStructureBuilderImpl& GateStructureBuilderImpl::withOr(const std::vector<Glucose::Lit>& inputs, Glucose::Lit output) {
        assertContainsVariable(m_gateVars, Glucose::var(output));
        assertDoesNotContainVariable(m_usedOutputs, Glucose::var(output));
        
        Cl bwd(inputs);
        bwd.push_back(~output);
        addClause(bwd);
        
        for (auto input : inputs) {
            addClause({~input, output});
        }
        
        insertVariables(inputs, m_gateVars);
        m_usedOutputs.insert(Glucose::var(output));
        return *this;
    }
    
    GateStructureBuilderImpl& GateStructureBuilderImpl::withXor(const std::vector<Glucose::Lit>&, Glucose::Lit) {
        // TODO: implement this
        assert(false);
        return *this;
    }
    
    std::unique_ptr<CNFProblem, void(*)(CNFProblem*)> GateStructureBuilderImpl::build() {
        m_usedOutputs.clear();
        m_gateVars.clear();
        m_usedOutputs.clear();
        return std::unique_ptr<CNFProblem, void(*)(CNFProblem*)>(m_builtClauses.release(), deleteClauses);
    }
    
    std::unique_ptr<GateStructureBuilder> createGateStructureBuilder() {
        return std::make_unique<GateStructureBuilderImpl>();
    }
    
    
}
