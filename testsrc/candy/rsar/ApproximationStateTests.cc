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

#define CANDY_EXPOSE_TESTING_INTERFACE

#include <core/SolverTypes.h>
#include <randomsimulation/Conjectures.h>
#include <rsar/ApproximationState.h>

#include <unordered_map>
#include <set>
#include <unordered_set>
#include <algorithm>



namespace Candy {
    static bool containsEquivalence(EquivalenceImplications& underTest, Lit a, Lit b);
    static bool containsLitAsAnteAndSucc(std::vector<Implication>& literals, Lit a);
    static std::pair<Lit,Lit> getSingleOccuringLiterals(std::vector<Implication>& implications);
    static bool contains(EquivalenceImplications& container, std::vector<Implication>& implications);
 
    template<typename X, typename Y>
    static bool contains(const X& iterable, const Y& thing) {
        return std::find(iterable.begin(), iterable.end(), thing) != iterable.end();
    }
    
    static EquivalenceImplications::CommitResult commitWorkQueue(EquivalenceImplications& target, bool withGenDelta) {
        if (withGenDelta) {
            return test_commitWorkQueue(target);
        }
        else {
            test_commitWorkQueueWithoutDelta(target);
            EquivalenceImplications::CommitResult empty;
            return empty;
        }
    }
    
    TEST(RSAREquivalenceImplications, createEmpty) {
        EquivalenceConjecture testData;
        auto underTest = test_createEquivalenceImplications(testData);
        
        EXPECT_TRUE(underTest->empty());
    }
    
    
    TEST(RSAREquivalenceImplications, createWithTwoVars) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        EXPECT_EQ(underTest->size(), 2ull);
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(0, 1), mkLit(10, 0)));
    }
    
    
    TEST(RSAREquivalenceImplications, createWithThreeVars) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        EXPECT_EQ(underTest->size(), 3ull);
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(0, 1), mkLit(100, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(0, 1), mkLit(10, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(10, 0), mkLit(0, 1)));
    }
    
    TEST(RSAREquivalenceImplications, removeInEmpty) {
        EquivalenceConjecture testData;
        auto underTest = test_createEquivalenceImplications(testData);
        
        underTest->addVariableRemovalToWorkQueue(1000);
        auto commitResult = test_commitWorkQueue(*underTest);
        
        EXPECT_TRUE(commitResult.newImplications.empty());
        EXPECT_TRUE(commitResult.removedImplications.empty());
        EXPECT_TRUE(underTest->empty());
    }
    
    
    TEST(RSAREquivalenceImplications, removeAllOfTwoVars) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        underTest->addVariableRemovalToWorkQueue(0);
        underTest->addVariableRemovalToWorkQueue(10);
        auto commitResult = test_commitWorkQueue(*underTest);
        
        EXPECT_TRUE(commitResult.newImplications.empty());
        EXPECT_EQ(commitResult.removedImplications.size(), 2ull);
        EXPECT_TRUE(underTest->empty());
        
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(0,1)));
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10,0)));
    }
    
    
    TEST(RSAREquivalenceImplications, removeOneOfTwoVars) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        underTest->addVariableRemovalToWorkQueue(0);
        auto commitResult = test_commitWorkQueue(*underTest);
        
        EXPECT_TRUE(commitResult.newImplications.empty());
        EXPECT_EQ(commitResult.removedImplications.size(), 2ull);
        EXPECT_TRUE(underTest->empty());
        
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(0,1)));
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10,0)));
    }
    
    
    TEST(RSAREquivalenceImplications, removeOneOfThreeVars) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        underTest->addVariableRemovalToWorkQueue(10);
        auto commitResult = test_commitWorkQueue(*underTest);
        
        EXPECT_EQ(commitResult.newImplications.size(), 1ull);
        EXPECT_EQ(commitResult.removedImplications.size(), 2ull);
        EXPECT_EQ(underTest->size(), 2ull);
        
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10,0)));
        auto holeBoundaries = getSingleOccuringLiterals(commitResult.removedImplications);
        EXPECT_TRUE(contains(testData, holeBoundaries.first));
        EXPECT_TRUE(contains(testData, holeBoundaries.second));
        EXPECT_TRUE(contains(*underTest, commitResult.newImplications));
        
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(0, 1), mkLit(100, 0)));
    }
    
    
    TEST(RSAREquivalenceImplications, removeTwoOfThreeVars) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        underTest->addVariableRemovalToWorkQueue(10);
        underTest->addVariableRemovalToWorkQueue(0);
        auto commitResult = test_commitWorkQueue(*underTest);
        
        EXPECT_TRUE(commitResult.newImplications.empty());
        EXPECT_EQ(commitResult.removedImplications.size(), 3ull);
        EXPECT_TRUE(underTest->empty());
        
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10,0)));
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(0,1)));
        EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(100,0)));
    }
    
    static void test_RSAREquivalenceImplications_removeAllOfThreeVars(bool withGenDelta) {
        EquivalenceConjecture testData;
        testData.addLit(mkLit(0, 1));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        auto underTest = test_createEquivalenceImplications(testData);
        
        underTest->addVariableRemovalToWorkQueue(10);
        underTest->addVariableRemovalToWorkQueue(0);
        underTest->addVariableRemovalToWorkQueue(100);
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_TRUE(underTest->empty());
        
        if (withGenDelta) {
            EXPECT_TRUE(commitResult.newImplications.empty());
            EXPECT_EQ(commitResult.removedImplications.size(), 3ull);
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10,0)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(0,1)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(100,0)));
        }
    }
    
    TEST(RSAREquivalenceImplications, removeAllOfThreeVars_genDelta) {
        test_RSAREquivalenceImplications_removeAllOfThreeVars(true);
    }
    
    TEST(RSAREquivalenceImplications, removeAllOfThreeVars_noGenDelta) {
        test_RSAREquivalenceImplications_removeAllOfThreeVars(false);
    }
    
    static void test_EquivalenceImplications_inflictOneSingleVariableHole(bool withGenDelta) {
        EquivalenceConjecture testData;
        
        testData.addLit(mkLit(1000, 0));
        testData.addLit(mkLit(10000, 0));
        testData.addLit(mkLit(100000, 0));
        testData.addLit(mkLit(1, 0));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        
        auto underTest = test_createEquivalenceImplications(testData);
        
        ASSERT_EQ(underTest->size(), 6ull);
        
        underTest->addVariableRemovalToWorkQueue(100);
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_EQ(underTest->size(), 5ull);
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(1, 0), mkLit(1000, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(1000, 0), mkLit(10000, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(10000, 0), mkLit(100000, 0)));

        if (withGenDelta) {
            EXPECT_EQ(commitResult.newImplications.size(), 1ull);
            EXPECT_EQ(commitResult.removedImplications.size(), 2ull);
            
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(100,0)));
            auto holeBoundaries = getSingleOccuringLiterals(commitResult.removedImplications);
            EXPECT_TRUE(contains(testData, holeBoundaries.first));
            EXPECT_TRUE(contains(testData, holeBoundaries.second));
            EXPECT_TRUE(contains(*underTest, commitResult.newImplications));
        }
    }
    
    TEST(RSAREquivalenceImplications, inflictOneSingleVariableHole_genDelta) {
        test_EquivalenceImplications_inflictOneSingleVariableHole(true);
    }
    
    TEST(RSAREquivalenceImplications, inflictOneSingleVariableHole_noGenDelta) {
        test_EquivalenceImplications_inflictOneSingleVariableHole(false);
    }
    
    static void test_EquivalenceImplications_inflictOneTreeVariableHole(bool withGenDelta) {
        // Note: This test assumes that implications are constructed such that
        // for all but one implication, var(A) < var(B) holds.
        
        EquivalenceConjecture testData;
        
        testData.addLit(mkLit(1000, 0));
        testData.addLit(mkLit(10000, 0));
        testData.addLit(mkLit(100000, 0));
        testData.addLit(mkLit(1, 0));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        
        auto underTest = test_createEquivalenceImplications(testData);
        
        ASSERT_EQ(underTest->size(), 6ull);
        
        underTest->addVariableRemovalToWorkQueue(1);
        underTest->addVariableRemovalToWorkQueue(10);
        underTest->addVariableRemovalToWorkQueue(100);
        
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_EQ(underTest->size(), 3ull);
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(1000, 0), mkLit(10000, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(10000, 0), mkLit(100000, 0)));
        
        if (withGenDelta) {
            EXPECT_EQ(commitResult.newImplications.size(), 1ull);
            EXPECT_EQ(commitResult.removedImplications.size(), 4ull);
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(1,0)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10,0)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(100,0)));
            auto holeBoundaries = getSingleOccuringLiterals(commitResult.removedImplications);
            EXPECT_TRUE(contains(testData, holeBoundaries.first));
            EXPECT_TRUE(contains(testData, holeBoundaries.second));
            EXPECT_TRUE(contains(*underTest, commitResult.newImplications));
        }
    }
    
    TEST(RSAREquivalenceImplications, inflictOneTreeVariableHole_genDelta) {
        test_EquivalenceImplications_inflictOneTreeVariableHole(true);
    }
    
    TEST(RSAREquivalenceImplications, inflictOneTreeVariableHole_noGenDelta) {
        test_EquivalenceImplications_inflictOneTreeVariableHole(false);
    }
    
    static void test_EquivalenceImplications_inflictTwoOneVariableHoles(bool withGenDelta) {
        // Note: This test assumes that implications are constructed such that
        // for all but one implication, var(A) < var(B) holds.
        
        EquivalenceConjecture testData;
        
        testData.addLit(mkLit(1000, 0));
        testData.addLit(mkLit(10000, 0));
        testData.addLit(mkLit(100000, 0));
        testData.addLit(mkLit(1, 0));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        
        auto underTest = test_createEquivalenceImplications(testData);
        
        ASSERT_EQ(underTest->size(), 6ull);
        
        underTest->addVariableRemovalToWorkQueue(1);
        underTest->addVariableRemovalToWorkQueue(1000);
        
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_EQ(underTest->size(), 4ull);
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(10, 0), mkLit(100, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(100, 0), mkLit(10000, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(10000, 0), mkLit(100000, 0)));
        
        if (withGenDelta) {
            EXPECT_EQ(commitResult.newImplications.size(), 2ull);
            EXPECT_EQ(commitResult.removedImplications.size(), 4ull);
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(1,0)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(1000,0)));
        
            /* TODO: check if the "boundaries of the holes" are valid lits */
            EXPECT_TRUE(contains(*underTest, commitResult.newImplications));
        }
    }
    
    TEST(RSAREquivalenceImplications, inflictTwoOneVariableHoles_genDelta) {
        test_EquivalenceImplications_inflictTwoOneVariableHoles(true);
    }
    
    TEST(RSAREquivalenceImplications, inflictTwoOneVariableHoles_noGenDelta) {
        test_EquivalenceImplications_inflictTwoOneVariableHoles(false);
    }
    
    static void test_EquivalenceImplicatoins_inflictMixedSizeVariableHoles(bool withGenDelta) {
        // Note: This test assumes that implications are constructed such that
        // for all but one implication, var(A) < var(B) holds.
        
        EquivalenceConjecture testData;
        
        testData.addLit(mkLit(1000, 0));
        testData.addLit(mkLit(10000, 0));
        testData.addLit(mkLit(100000, 0));
        testData.addLit(mkLit(1, 0));
        testData.addLit(mkLit(10, 0));
        testData.addLit(mkLit(100, 0));
        
        auto underTest = test_createEquivalenceImplications(testData);
        
        ASSERT_EQ(underTest->size(), 6ull);
        
        underTest->addVariableRemovalToWorkQueue(1);
        underTest->addVariableRemovalToWorkQueue(1000);
        underTest->addVariableRemovalToWorkQueue(10000);
        
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_EQ(underTest->size(), 3ull);
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(10, 0), mkLit(100, 0)));
        EXPECT_TRUE(containsEquivalence(*underTest, mkLit(100, 0), mkLit(100000, 0)));

        if (withGenDelta) {
            EXPECT_EQ(commitResult.newImplications.size(), 2ull);
            EXPECT_EQ(commitResult.removedImplications.size(), 5ull);
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(1,0)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(1000,0)));
            EXPECT_TRUE(containsLitAsAnteAndSucc(commitResult.removedImplications, mkLit(10000,0)));
        
            /* TODO: check if the "boundaries of the holes" are valid lits */
        
            EXPECT_TRUE(contains(*underTest, commitResult.newImplications));
        }
    }
    
    TEST(RSAREquivalenceImplications, inflictMixedSizeVariableHoles_genDelta) {
        test_EquivalenceImplicatoins_inflictMixedSizeVariableHoles(true);
    }
    
    TEST(RSAREquivalenceImplications, inflictMixedSizeVariableHoles_noGenDelta) {
        test_EquivalenceImplicatoins_inflictMixedSizeVariableHoles(false);
    }
    
    
    static Backbones::CommitResult commitWorkQueue(Backbones& target, bool withGenDelta) {
        if (withGenDelta) {
            return test_commitWorkQueue(target);
        }
        else {
            test_commitWorkQueue(target);
            Backbones::CommitResult empty;
            return empty;
        }
    }
    
    TEST(RSARBackbones, createEmpty) {
        std::vector<BackboneConjecture> testData;
        auto underTest = test_createBackbones(testData);
        
        EXPECT_TRUE(underTest->empty());
    }
    
    
    TEST(RSARBackbones, createSingle) {
        std::vector<BackboneConjecture> testData;
        testData.push_back(BackboneConjecture(mkLit(1,0)));
        auto underTest = test_createBackbones(testData);
        
        EXPECT_EQ(underTest->size(), 1ull);
        EXPECT_TRUE(contains(*underTest, mkLit(1,0)));
    }
    
    TEST(RSARBackbones, createTriple) {
        std::vector<BackboneConjecture> testData;
        testData.push_back(BackboneConjecture(mkLit(1,0)));
        testData.push_back(BackboneConjecture(mkLit(2,0)));
        testData.push_back(BackboneConjecture(mkLit(3,0)));
        auto underTest = test_createBackbones(testData);
        
        EXPECT_EQ(underTest->size(), 3ull);
        EXPECT_TRUE(contains(*underTest, mkLit(1,0)));
        EXPECT_TRUE(contains(*underTest, mkLit(2,0)));
        EXPECT_TRUE(contains(*underTest, mkLit(3,0)));
    }
    
    static void test_Backbones_removeInEmpty(bool withGenDelta) {
        std::vector<BackboneConjecture> testData;
        auto underTest = test_createBackbones(testData);
        
        underTest->addVariableRemovalToWorkQueue(10);
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_TRUE(underTest->empty());
        
        if (withGenDelta) {
            EXPECT_TRUE(commitResult.removedBackbones.empty());
        }
    }
    
    
    TEST(RSARBackbones, removeInEmpty_genDelta) {
        test_Backbones_removeInEmpty(true);
    }
    
    TEST(RSARBackbones, removeInEmpty_noDelta) {
        test_Backbones_removeInEmpty(false);
    }
    
    static void test_Backbones_removeAll(bool withGenDelta) {
        std::vector<BackboneConjecture> testData;
        testData.push_back(BackboneConjecture(mkLit(1,0)));
        testData.push_back(BackboneConjecture(mkLit(2,0)));
        testData.push_back(BackboneConjecture(mkLit(3,0)));
        
        auto underTest = test_createBackbones(testData);
        
        underTest->addVariableRemovalToWorkQueue(1);
        underTest->addVariableRemovalToWorkQueue(2);
        underTest->addVariableRemovalToWorkQueue(3);
        
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_TRUE(underTest->empty());
        
        if (withGenDelta) {
            EXPECT_EQ(commitResult.removedBackbones.size(), 3ull);
            EXPECT_TRUE(contains(commitResult.removedBackbones, mkLit(1,0)));
            EXPECT_TRUE(contains(commitResult.removedBackbones, mkLit(2,0)));
            EXPECT_TRUE(contains(commitResult.removedBackbones, mkLit(3,0)));
        }
    }
    
    TEST(RSARBackbones, removeAll_genDelta) {
        test_Backbones_removeAll(true);
    }
    
    TEST(RSARBackbones, removeAll_noDelta) {
        test_Backbones_removeAll(false);
    }
    
    static void test_Backbones_removeAllButOne(bool withGenDelta) {
        std::vector<BackboneConjecture> testData;
        testData.push_back(BackboneConjecture(mkLit(1,0)));
        testData.push_back(BackboneConjecture(mkLit(2,0)));
        testData.push_back(BackboneConjecture(mkLit(3,0)));
        
        auto underTest = test_createBackbones(testData);
        
        underTest->addVariableRemovalToWorkQueue(1);
        underTest->addVariableRemovalToWorkQueue(2);
        
        auto commitResult = commitWorkQueue(*underTest, withGenDelta);
        
        EXPECT_EQ(underTest->size(), 1ull);
        
        if (withGenDelta) {
            EXPECT_EQ(commitResult.removedBackbones.size(), 2ull);
        
        
            EXPECT_TRUE(contains(commitResult.removedBackbones, mkLit(1,0)));
            EXPECT_TRUE(contains(commitResult.removedBackbones, mkLit(2,0)));
            EXPECT_TRUE(contains(*underTest, mkLit(3,0)));
        }
    }
    
    TEST(RSARBackbones, removeAllButOne_genDelta) {
        test_Backbones_removeAllButOne(true);
    }
    
    TEST(RSARBackbones, removeAllButOne_noGenDelta) {
        test_Backbones_removeAllButOne(false);
    }
    
    
    
    static void test_ApproximationState_emptyConjectures(bool testInitDelta) {
        Conjectures testData;
        auto underTest = createApproximationState(testData);
        
        EXPECT_TRUE(underTest->beginEquivalenceImplications() == underTest->endEquivalenceImplications());
        EXPECT_TRUE(underTest->getBackbones().empty());
        
        auto refDelta = (testInitDelta ? underTest->createInitializationDelta() : underTest->createDelta());
        EXPECT_TRUE(refDelta->beginAddedImplications() == refDelta->endAddedImplications());
        EXPECT_TRUE(refDelta->beginRemovedImplications() == refDelta->endRemovedImplications());
        EXPECT_TRUE(refDelta->beginRemovedBackbones() == refDelta->endRemovedBackbones());
    }
    
    TEST(RSARApproximationState, emptyConjectures_initDelta) {
        test_ApproximationState_emptyConjectures(true);
    }
    
    TEST(RSARApproximationState, emptyConjectures_runDelta) {
        test_ApproximationState_emptyConjectures(false);
    }
    
    
    
    static void test_ApproximationState_singleConjectures(bool testInitDelta, bool remove) {
        Conjectures testData;
        
        EquivalenceConjecture eqc;
        eqc.addLit(mkLit(0, 1));
        eqc.addLit(mkLit(1, 0));
        
        BackboneConjecture bbc(mkLit(2, 0));
        
        testData.addEquivalence(eqc);
        testData.addBackbone(bbc);
        
        auto underTest = createApproximationState(testData);
        
        ASSERT_EQ(underTest->getBackbones().size(), 1ull);
        ASSERT_EQ(underTest->equivalenceImplicationsSize(), 1ull);
        
        if (remove) {
            (*underTest->beginEquivalenceImplications())->addVariableRemovalToWorkQueue(0);
        }
        
        auto refDelta = (testInitDelta ? underTest->createInitializationDelta() : underTest->createDelta());
        
        EXPECT_EQ(refDelta->addedImplicationsSize(), (testInitDelta && !remove) ? 2ull : 0ull);
        EXPECT_EQ(refDelta->removedBackbonesSize(), 0ull);
        EXPECT_EQ(refDelta->removedImplicationsSize(), (!testInitDelta && remove) ? 2ull : 0ull);
    }
    
    
    TEST(RSARApproximationState, noRemove_singleConjectures_initDelta) {
        test_ApproximationState_singleConjectures(true, false);
    }
    
    TEST(RSARApproximationState, noRemove_singleConjectures_runDelta) {
        test_ApproximationState_singleConjectures(false, false);
    }
    
    TEST(RSARApproximationState, remove_singleConjectures_initDelta) {
        test_ApproximationState_singleConjectures(true, true);
    }
    
    TEST(RSARApproximationState, remove_singleConjectures_runDelta) {
        test_ApproximationState_singleConjectures(false, true);
    }
    
    
    
    
    static void test_ApproximationState_multiConjectures(bool testInitDelta, bool remove) {
        Conjectures testData;
        
        EquivalenceConjecture eqc;
        eqc.addLit(mkLit(0, 1));
        eqc.addLit(mkLit(1, 0));
        
        EquivalenceConjecture eqc2;
        eqc2.addLit(mkLit(2, 1));
        eqc2.addLit(mkLit(3, 0));
        eqc2.addLit(mkLit(6, 0));
        
        BackboneConjecture bbc(mkLit(4, 0));
        BackboneConjecture bbc2(mkLit(5, 0));
        
        testData.addEquivalence(eqc);
        testData.addEquivalence(eqc2);
        testData.addBackbone(bbc);
        testData.addBackbone(bbc2);
        
        auto underTest = createApproximationState(testData);
        
        ASSERT_EQ(underTest->getBackbones().size(), 2ull);
        ASSERT_EQ(underTest->equivalenceImplicationsSize(), 2ull);
        
        if (remove) {
            auto eqIter = underTest->beginEquivalenceImplications();
            (*eqIter)->addVariableRemovalToWorkQueue(2);
            ++eqIter;
            (*eqIter)->addVariableRemovalToWorkQueue(2);
            
            underTest->getBackbones().addVariableRemovalToWorkQueue(4);
        }
        
        auto refDelta = (testInitDelta ? underTest->createInitializationDelta() : underTest->createDelta());
        
        EXPECT_EQ(refDelta->addedImplicationsSize(), testInitDelta ? (remove ? 4ull : 5ull) : (remove ? 1ull : 0ull));
        EXPECT_EQ(refDelta->removedBackbonesSize(), (!testInitDelta && remove) ? 1ull : 0ull);
        EXPECT_EQ(refDelta->removedImplicationsSize(), (!testInitDelta && remove) ? 2ull : 0ull);
    }
    
    
    TEST(RSARApproximationState, noRemove_multiConjectures_initDelta) {
        test_ApproximationState_multiConjectures(true, false);
    }
    
    TEST(RSARApproximationState, noRemove_multiConjectures_runDelta) {
        test_ApproximationState_multiConjectures(false, false);
    }
    
    TEST(RSARApproximationState, remove_multiConjectures_initDelta) {
        test_ApproximationState_multiConjectures(true, true);
    }
    
    TEST(RSARApproximationState, remove_multiConjectures_runDelta) {
        test_ApproximationState_multiConjectures(false, true);
    }
    
    
    
    
    static bool containsEquivalence(EquivalenceImplications& underTest, Lit a, Lit b) {
        std::unordered_map<Lit,Lit> implicationMap;
        for (auto i : underTest) {
            implicationMap[i.first] = i.second;
        }
        
        if (implicationMap.find(a) == implicationMap.end()) {
            return false;
        }
        
        if (implicationMap[a] == a) {
            return true;
        }
        
        Lit cursor = implicationMap[a];
        while (cursor != a && cursor != b) {
            cursor = implicationMap[cursor];
        }
        
        return (cursor == b);
    }
    
    
    static bool containsLitAsAnteAndSucc(std::vector<Implication>& literals, Lit a) {
        auto foundAnte = std::find_if(literals.begin(), literals.end(),
                                      [a](Implication i) { return a == i.first; });
        auto foundSucc = std::find_if(literals.begin(), literals.end(),
                                      [a](Implication i) { return a == i.second; });
        return foundAnte != literals.end() && foundSucc != literals.end();
    }
    
    
    static std::pair<Lit,Lit> getSingleOccuringLiterals(std::vector<Implication>& implications) {
        std::set<Lit> seen;
        for (auto&& impl : implications) {
            if (seen.find(impl.first) == seen.end()) {
                seen.insert(impl.first);
            }
            else {
                seen.erase(impl.first);
            }
            
            if (seen.find(impl.second) == seen.end()) {
                seen.insert(impl.second);
            }
            else {
                seen.erase(impl.second);
            }
        }
        
        assert(seen.size() == 2ull);
        
        auto seenIter = seen.begin();
        Lit l1 = *seenIter;
        ++seenIter;
        Lit l2 = *seenIter;
        
        return std::pair<Lit,Lit>(l1, l2);
    }
    
    static bool contains(EquivalenceImplications& container, std::vector<Implication>& implications) {
        std::unordered_set<Implication> work;
        work.insert(implications.begin(), implications.end());
        
        for (auto impl : container) {
            work.erase(impl);
        }
        
        return work.size() == 0ull;
    }
}
