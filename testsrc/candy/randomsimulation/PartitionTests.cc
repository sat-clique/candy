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

#include <limits>
#include <algorithm>

#include "TestUtils.h"

#include <randomsimulation/Partition.h>
#include <randomsimulation/Conjectures.h>
#include <randomsimulation/SimulationVector.h>
#include <core/SolverTypes.h>


namespace Candy {
    
    static void setAssignmentPattern(SimulationVectors &assignment,
                              SimulationVectors::index_t index,
                              SimulationVector::varsimvec_field_t pattern) {
        for (size_t i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
            assignment.get(index).vars[i] = pattern;
        }
    }
    
    static void test_singleUpdateAllRelevantAllInConjs(std::unique_ptr<Partition> underTest) {
        std::vector<Glucose::Var> relevantVars = {0, 1, 2};
        
        SimulationVectors assignment;
        assignment.initialize(3);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0xABABABull);
        setAssignmentPattern(assignment, 2, ~(0xABABABull));
        
        underTest->setVariables(relevantVars);
        underTest->update(assignment);
        
        auto conjectures = underTest->getConjectures();
        EXPECT_EQ(conjectures.getBackbones().size(), 1ull);
        ASSERT_EQ(conjectures.getEquivalences().size(), 1ull);
        
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(0, 1)));
        
        std::vector<Glucose::Lit> eqConj1 = {Glucose::mkLit(1, 0), Glucose::mkLit(2, 1)};
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj1));
    }
    
    TEST (RSPartitionTest, singleUpdateAllRelevantAllInConjs_noCompress) {
        test_singleUpdateAllRelevantAllInConjs(createDefaultPartition(createNullCompressionScheduleStrategy()));
    }
    
    TEST (RSPartitionTest, singleUpdateAllRelevantAllInConjs_linCompress) {
        test_singleUpdateAllRelevantAllInConjs(createDefaultPartition(createLinearCompressionScheduleStrategy(1)));
    }
    
    TEST (RSPartitionTest, singleUpdateAllRelevantAllInConjs_logCompress) {
        test_singleUpdateAllRelevantAllInConjs(createDefaultPartition(createLogCompressionScheduleStrategy()));
    }
    
    
    static void test_multiUpdateAllRelevantAllInConjs(std::unique_ptr<Partition> underTest) {
        std::vector<Glucose::Var> relevantVars = {0, 1, 2};
        
        SimulationVectors assignment;
        assignment.initialize(3);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0xABABABull);
        setAssignmentPattern(assignment, 2, ~(0xABABABull));
        
        underTest->setVariables(relevantVars);
        underTest->update(assignment);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0x10ull);
        setAssignmentPattern(assignment, 2, ~(0x10ull));
        
        underTest->update(assignment);
        
        auto conjectures = underTest->getConjectures();
        EXPECT_EQ(conjectures.getBackbones().size(), 1ull);
        ASSERT_EQ(conjectures.getEquivalences().size(), 1ull);
        
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(0, 1)));
        
        std::vector<Glucose::Lit> eqConj1 = {Glucose::mkLit(1, 0), Glucose::mkLit(2, 1)};
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj1));
    }
    
    TEST (RSPartitionTest, multiUpdateAllRelevantAllInConjs_noCompress) {
        test_multiUpdateAllRelevantAllInConjs(createDefaultPartition(createNullCompressionScheduleStrategy()));
    }
    
    TEST (RSPartitionTest, multiUpdateAllRelevantAllInConjs_linCompress) {
        test_multiUpdateAllRelevantAllInConjs(createDefaultPartition(createLinearCompressionScheduleStrategy(1)));
    }
    
    TEST (RSPartitionTest, multiUpdateAllRelevantAllInConjs_logCompress) {
        test_multiUpdateAllRelevantAllInConjs(createDefaultPartition(createLogCompressionScheduleStrategy()));
    }
    
    static void test_singleUpdateAllRelevant(std::unique_ptr<Partition> underTest) {
        std::vector<Glucose::Var> relevantVars = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        
        SimulationVectors assignment;
        assignment.initialize(10);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0ull);
        
        setAssignmentPattern(assignment, 2, 0x5ull);
        setAssignmentPattern(assignment, 4, 0x5ull);
        
        setAssignmentPattern(assignment, 5, 0x2342ull);
        setAssignmentPattern(assignment, 6, 0x5346ull);
        setAssignmentPattern(assignment, 8, 0x51346ull);
        
        setAssignmentPattern(assignment, 3, 0xABABABull);
        setAssignmentPattern(assignment, 7, ~(0xABABABull));
        setAssignmentPattern(assignment, 9, ~(0xABABABull));
        
        underTest->setVariables(relevantVars);
        underTest->update(assignment);
        
        auto conjectures = underTest->getConjectures();
        EXPECT_EQ(conjectures.getBackbones().size(), 2ull);
        ASSERT_EQ(conjectures.getEquivalences().size(), 2ull);
        
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(0, 1)));
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(1, 0)));
        
        std::vector<Glucose::Lit> eqConj1 = {Glucose::mkLit(3, 0),
            Glucose::mkLit(7, 1),
            Glucose::mkLit(9, 1)};
        std::vector<Glucose::Lit> eqConj2 = {Glucose::mkLit(2, 0),
            Glucose::mkLit(4, 0)};
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj1));
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj2));
    }
    
    TEST (RSPartitionTest, singleUpdateAllRelevant_noCompress) {
        test_singleUpdateAllRelevant(createDefaultPartition(createNullCompressionScheduleStrategy()));
    }
    
    TEST (RSPartitionTest, singleUpdateAllRelevant_linCompress) {
        test_singleUpdateAllRelevant(createDefaultPartition(createLinearCompressionScheduleStrategy(1)));
    }
    
    TEST (RSPartitionTest, singleUpdateAllRelevant_logCompress) {
        test_singleUpdateAllRelevant(createDefaultPartition(createLogCompressionScheduleStrategy()));
    }
    
    void test_multiUpdateAllRelevant(std::unique_ptr<Partition> underTest) {
        std::vector<Glucose::Var> relevantVars = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        
        SimulationVectors assignment;
        assignment.initialize(10);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0ull);
        
        setAssignmentPattern(assignment, 2, 0x5ull);
        setAssignmentPattern(assignment, 4, 0x5ull);
        
        setAssignmentPattern(assignment, 5, 0x5ull);
        setAssignmentPattern(assignment, 6, 0x5346ull);
        setAssignmentPattern(assignment, 8, ~(0x5ull));
        
        setAssignmentPattern(assignment, 3, 0xABABABull);
        setAssignmentPattern(assignment, 7, ~(0xABABABull));
        setAssignmentPattern(assignment, 9, ~(0xABABABull));
        
        underTest->setVariables(relevantVars);
        underTest->update(assignment);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0ull);
        
        setAssignmentPattern(assignment, 2, 0x72ull);
        setAssignmentPattern(assignment, 4, 0x72ull);
        
        setAssignmentPattern(assignment, 5, 0x23234242ull);
        setAssignmentPattern(assignment, 6, 0x5325546ull);
        setAssignmentPattern(assignment, 8, ~(0x5ull));
        
        setAssignmentPattern(assignment, 3, 0xC32FFFFFull);
        setAssignmentPattern(assignment, 7, ~(0xC32FFFFFull));
        setAssignmentPattern(assignment, 9, ~(0xC32FFFFFull));
        
        underTest->update(assignment);
        underTest->update(assignment);
        setAssignmentPattern(assignment, 8, ~(0x1ull));
        underTest->update(assignment);
        underTest->update(assignment);

        
        auto conjectures = underTest->getConjectures();
        EXPECT_EQ(conjectures.getBackbones().size(), 2ull);
        ASSERT_EQ(conjectures.getEquivalences().size(), 2ull);
        
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(0, 1)));
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(1, 0)));
        
        std::vector<Glucose::Lit> eqConj1 = {Glucose::mkLit(3, 0),
            Glucose::mkLit(7, 1),
            Glucose::mkLit(9, 1)};
        std::vector<Glucose::Lit> eqConj2 = {Glucose::mkLit(2, 0),
            Glucose::mkLit(4, 0)};
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj1));
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj2));
    }
    
    TEST (RSPartitionTest, multiUpdateAllRelevant_noCompress) {
        test_multiUpdateAllRelevant(createDefaultPartition(createNullCompressionScheduleStrategy()));
    }
    
    TEST (RSPartitionTest, multiUpdateAllRelevant_linCompress) {
        test_multiUpdateAllRelevant(createDefaultPartition(createLinearCompressionScheduleStrategy(1)));
    }
    
    TEST (RSPartitionTest, multiUpdateAllRelevant_logCompress) {
        test_multiUpdateAllRelevant(createDefaultPartition(createLogCompressionScheduleStrategy()));
    }
    
    
    static void test_multipleBackboneVars(std::unique_ptr<Partition> underTest) {
        std::vector<Glucose::Var> relevantVars = {0, 1, 2, 3, 4};
        
        SimulationVectors assignment;
        assignment.initialize(5);
        
        setAssignmentPattern(assignment, 0, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 1, 0xABABABull);
        setAssignmentPattern(assignment, 2, ~(0xABABABull));
        setAssignmentPattern(assignment, 3, std::numeric_limits<SimulationVector::varsimvec_field_t>::max());
        setAssignmentPattern(assignment, 4, 0ull);
        
        underTest->setVariables(relevantVars);
        underTest->update(assignment);
        
        auto conjectures = underTest->getConjectures();
        EXPECT_EQ(conjectures.getBackbones().size(), 3ull);
        ASSERT_EQ(conjectures.getEquivalences().size(), 1ull);
        
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(0, 1)));
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(3, 1)));
        EXPECT_TRUE(hasBackboneConj(conjectures, Glucose::mkLit(4, 0)));
        
        std::vector<Glucose::Lit> eqConj1 = {Glucose::mkLit(1, 0), Glucose::mkLit(2, 1)};
        EXPECT_TRUE(hasEquivalenceConj(conjectures, eqConj1));
    }
    
    TEST (RSPartitionTest, multipleBackboneVars_noCompress) {
        test_multipleBackboneVars(createDefaultPartition(createNullCompressionScheduleStrategy()));
    }
    
    TEST (RSPartitionTest, multipleBackboneVars_linCompress) {
        test_multipleBackboneVars(createDefaultPartition(createLinearCompressionScheduleStrategy(1)));
    }
    
    TEST (RSPartitionTest, multipleBackboneVars_logCompress) {
        test_multipleBackboneVars(createDefaultPartition(createLogCompressionScheduleStrategy()));
    }
}
