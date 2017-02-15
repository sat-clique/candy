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
#include <gates/GateAnalyzer.h>
#include <rsar/Heuristics.h>
#include <rsar/ApproximationState.h>

#include <testutils/TestGateStructure.h>

#include <unordered_set>
#include <vector>

namespace Candy {
    class FakeEquivalenceImplications : public EquivalenceImplications {
    public:
        void addVariableRemovalToWorkQueue(Var it) noexcept override {
            m_variableRemovalWorkQueue.insert(it);
        }
        
        /** Begin iterator for implications. Gets invalidated when the work queue is committed. */
        const_iterator begin() const noexcept override {
            return m_impls.begin();
        }
        
        /** End iterator for implications. Gets invalidated when the work queue is committed. */
        const_iterator end() const noexcept override {
            return m_impls.end();
        }
        
        /** Returns the amount of implications stored in this object. */
        size_type size() const noexcept override {
            return m_impls.size();
        }
        
        /** Returns true iff no implications are stored in this object. */
        bool empty() const noexcept override {
            return m_impls.empty();
        }
        
        /** Returns the n'th stored implication. */
        virtual Implication at(size_type index) const override {
            return m_impls[index];
        }
        
        
        void clearVariableRemovalQueue() {
            m_variableRemovalWorkQueue.clear();
        }
        
        const std::unordered_set<Var>& getVariableRemovalQueue() const {
            return m_variableRemovalWorkQueue;
        }
        
        void setLiterals(const std::vector<Lit>& literals) {
            assert(literals.size() >= 2);
            
            m_impls.clear();
            for (size_t i = 0; i < literals.size()-1; ++i) {
                m_impls.push_back({literals[i], literals[i+1]});
            }
            m_impls.push_back({literals[literals.size()-1], literals[0]});
        }
        
        virtual ~FakeEquivalenceImplications() {
        }
        
    private:
        std::vector<Implication> m_impls;
        std::unordered_set<Var> m_variableRemovalWorkQueue;
    };
    
    
    TEST(RSARRefinementHeuristicsTests, InputDepCount_deactivatesThreeGatesSeq) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        gateBuilder->withAnd({Glucose::mkLit(3, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(1,1));
        gateBuilder->withAnd({Glucose::mkLit(5, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(3,1));
        auto problem = gateBuilder->build();
        GateAnalyzer analyzer{*problem};
        analyzer.analyze();
        
        auto underTest = createInputDepCountRefinementHeuristic(analyzer, {4,2});
        underTest->beginRefinementStep();
        
        FakeEquivalenceImplications fakeEq;
        fakeEq.setLiterals({Glucose::mkLit(0, 1),
            Glucose::mkLit(0, 1),
            Glucose::mkLit(1, 1),
            Glucose::mkLit(3, 1)});
        
        // initialization: no removals under this config
        underTest->markRemovals(fakeEq);
        ASSERT_EQ(fakeEq.getVariableRemovalQueue().size(), 0ull);
        
        // refinement 1: remove 0
        underTest->beginRefinementStep();
        underTest->markRemovals(fakeEq);
        auto removalsStep2 = fakeEq.getVariableRemovalQueue();
        ASSERT_EQ(removalsStep2.size(), 1ull);
        ASSERT_TRUE(removalsStep2.find(0) != removalsStep2.end());
        fakeEq.clearVariableRemovalQueue();
        
        // refinement 2: remove 1,3
        underTest->beginRefinementStep();
        underTest->markRemovals(fakeEq);
        auto removalsStep3 = fakeEq.getVariableRemovalQueue();
        ASSERT_EQ(removalsStep3.size(), 2ull);
        ASSERT_TRUE(removalsStep3.find(1) != removalsStep3.end());
        ASSERT_TRUE(removalsStep3.find(3) != removalsStep3.end());
    }
    
    TEST(RSARRefinementHeuristicsTests, InputDepCount_deactivatesManyGatesSeq) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        gateBuilder->withAnd({Glucose::mkLit(3, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(1,1));
        gateBuilder->withAnd({Glucose::mkLit(5, 1), Glucose::mkLit(4, 1), Glucose::mkLit(6, 1)}, Glucose::mkLit(3,0));
        gateBuilder->withAnd({Glucose::mkLit(7, 0), Glucose::mkLit(6, 0)}, Glucose::mkLit(4,1));
        gateBuilder->withOr({Glucose::mkLit(8, 1), Glucose::mkLit(9,0), Glucose::mkLit(5,0)}, Glucose::mkLit(7,0));
        gateBuilder->withAnd({Glucose::mkLit(10, 1), Glucose::mkLit(9,0), Glucose::mkLit(6,0)}, Glucose::mkLit(8,0));
        gateBuilder->withAnd({Glucose::mkLit(11, 1), Glucose::mkLit(12,0), Glucose::mkLit(13,0)}, Glucose::mkLit(5,0));

        // Input depenency counts by variable:
        // 0:7; 1:6; 3:6; 4:6; 5:3; 7:6; 8:3
        
        auto problem = gateBuilder->build();
        GateAnalyzer analyzer{*problem};
        analyzer.analyze();
        
        auto underTest = createInputDepCountRefinementHeuristic(analyzer, {10, 6, 5, 2});
        underTest->beginRefinementStep();
        
        FakeEquivalenceImplications fakeEq;
        fakeEq.setLiterals({Glucose::mkLit(0, 1),
            Glucose::mkLit(0, 1),
            Glucose::mkLit(1, 1),
            Glucose::mkLit(3, 1),
            Glucose::mkLit(4, 1),
            Glucose::mkLit(5, 1),
            Glucose::mkLit(6, 1),
            Glucose::mkLit(7, 1),
            Glucose::mkLit(8, 1)});
        
        // initialization: no removals under this config
        underTest->markRemovals(fakeEq);
        ASSERT_EQ(fakeEq.getVariableRemovalQueue().size(), 0ull);
        
        // refinement 1: remove 0
        underTest->beginRefinementStep();
        underTest->markRemovals(fakeEq);
        auto removalsStep2 = fakeEq.getVariableRemovalQueue();
        ASSERT_EQ(removalsStep2.size(), 1ull);
        ASSERT_TRUE(removalsStep2.find(0) != removalsStep2.end());
        fakeEq.clearVariableRemovalQueue();
        
        // refinement 2: remove 1,3,4,7
        underTest->beginRefinementStep();
        underTest->markRemovals(fakeEq);
        auto removalsStep3 = fakeEq.getVariableRemovalQueue();
        ASSERT_EQ(removalsStep3.size(), 4ull);
        ASSERT_TRUE(removalsStep3.find(1) != removalsStep3.end());
        ASSERT_TRUE(removalsStep3.find(3) != removalsStep3.end());
        ASSERT_TRUE(removalsStep3.find(4) != removalsStep3.end());
        ASSERT_TRUE(removalsStep3.find(7) != removalsStep3.end());
        fakeEq.clearVariableRemovalQueue();
        
        // refinement 3: remove 5,8
        underTest->beginRefinementStep();
        underTest->markRemovals(fakeEq);
        auto removalsStep4 = fakeEq.getVariableRemovalQueue();
        ASSERT_EQ(removalsStep4.size(), 2ull);
        ASSERT_TRUE(removalsStep4.find(5) != removalsStep4.end());
        ASSERT_TRUE(removalsStep4.find(8) != removalsStep4.end());
        fakeEq.clearVariableRemovalQueue();
        
        // refinement 4: remove nothing
        underTest->beginRefinementStep();
        underTest->markRemovals(fakeEq);
        ASSERT_TRUE(fakeEq.getVariableRemovalQueue().empty());
    }
}
