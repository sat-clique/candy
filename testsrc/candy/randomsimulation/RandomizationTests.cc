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
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <bitset>
#include <cmath>

#include <randomsimulation/Randomization.h>
#include <randomsimulation/SimulationVector.h>
#include <testutils/TestUtils.h>

namespace Candy {
    // TODO: improve documentation
    
    /** Interpreting the argument testData as a sequence of 4-bit values, this
     * function computes a map mapping nibbles to their occurence count within testData.
     */
    static std::unordered_map<std::uint8_t, std::uint64_t>
    getNibbleOccurences(const std::vector<SimulationVector::varsimvec_field_t>& testData) {
        std::unordered_map<std::uint8_t, std::uint64_t> result{};
        
        for (uint8_t i = 0; i < 16; ++i) {
            result[i] = 0ull;
        }
        
        for (auto x : testData) {
            for (size_t i = 0; i < 2*sizeof(x); ++i) {
                SimulationVector::varsimvec_field_t mask = 0xFull << 4*i;
                SimulationVector::varsimvec_field_t val = (x & mask) >> 4*i;
                assert(val < 256ull);
                result[(uint8_t)val]++;
            }
        }
        
        return result;
    }
    
    /** Given the nibble-to-occurence-count map \p{occurences}, this function computes the
     * corresponding empirical cumulative distribution function (i.e. nibble vs.
     * cumulative probability of occurence).
     */
    static std::unordered_map<std::uint8_t, double>
    getECDF(const std::unordered_map<std::uint8_t, std::uint64_t>& occurences) {
        std::uint64_t sampleSize = 0;
        for (auto& kv : occurences) {
            sampleSize += kv.second;
        }
        
        std::unordered_map<std::uint8_t, double> result {};
        std::uint64_t accu = 0ull;
        for (std::uint8_t i = 0; i < 16; ++i) {
            accu += occurences.at(i);
            result[i] = ((double)accu)/((double)sampleSize);
        }
        
        return result;
    }
    

    
    /**
     * Gets the reference empirical cumulative distribution function for 4-bit samples and
     * the given bias.
     * If the given bias is positive, the sample is computed for the rsp. biased-to-true
     * distribution; if the bias is negative, the sample is computed for the rsp.
     * bias-to-false distribution with a bias of -bias.
     */
    static std::unordered_map<std::uint8_t, double> getReferenceECDF(int bias) {
        double probabilityOfTrue;
        if (bias > 0) {
            probabilityOfTrue = 1.0f - (std::exp2((double)(-bias)));
        }
        else {
            probabilityOfTrue = std::exp2((double)(bias));
        }
        
        std::unordered_map<std::uint8_t, double> individualProbs {};
        for (std::uint8_t i = 0; i < 16; ++i) {
            std::bitset<4> iBits {i};
            double ones = iBits.count();
            individualProbs[i] = std::pow(probabilityOfTrue, ones) * std::pow(1.0f-probabilityOfTrue, 4-ones);
        }
        
        std::unordered_map<std::uint8_t, double> result {};
        for (std::uint8_t i = 0; i < 16; ++i) {
            for (std::uint8_t j = 0; j <= i; ++j) {
                result[i] += individualProbs[j];
            }
        }
        
        return result;
    }
    
    /**
     * Performs a simple Kolmogorov–Smirnov 2-sample test on the given RNG
     * (measured sample empirical cumulative distribution function vs.
     * reference sample empirical distribution function).
     * The test is performed by interpreting the RNG output as a stream of
     * 4-bit samples.
     *
     * \param underTest                 The RNG to test
     * \param referenceDistribution     The reference distribution function
     * \param measurementStart          Gives the number of the first RNG invocation to sample
     * \param measurementPeriod         The period of RNG sample measurements
     * \param measurements              The number of RNG values to sample
     * \param maxKS                     The maximum allowed value of the Kolmogorov–Smirnov test
     */
    static void test_randomization(Randomization& underTest,
                                   const std::unordered_map<std::uint8_t, double>& referenceDistribution,
                                   int measurementStart,
                                   int measurementPeriod,
                                   int measurements,
                                   double maxKS) {
        SimulationVectors testVectors;
        testVectors.initialize(1);
        
        
        // gather test data
        std::vector<SimulationVector::varsimvec_field_t> sample;
        for (int i = 0; i < measurementStart + (measurementPeriod * measurements); ++i) {
            underTest.randomize(testVectors, {0});
            
            if (i >= measurementStart && (i - measurementStart) % measurementPeriod == 0) {
                for (size_t i = 0; i < SimulationVector::VARSIMVECSIZE; ++i) {
                    sample.push_back(testVectors.get(0).vars[i]);
                }
            }
        }
        
        auto occurences = getNibbleOccurences(sample);
        auto sampleDistribution = getECDF(occurences);
        
        double ks = getMaxAbsDifference(sampleDistribution, referenceDistribution);
        
        EXPECT_LE(ks, maxKS);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_SimpleRandomization) {
        auto rng = createSimpleRandomization();
        auto referenceDist = getReferenceECDF(1);
        test_randomization(*rng, referenceDist, 0, 1, 1000, 0.01f);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_PositivelyBiasedRandomization_Bias1) {
        auto rng = createRandomizationBiasedToTrue(1);
        auto referenceDist = getReferenceECDF(1);
        test_randomization(*rng, referenceDist, 0, 1, 1000, 0.01f);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_PositivelyBiasedRandomization_Bias3) {
        auto rng = createRandomizationBiasedToTrue(3);
        auto referenceDist = getReferenceECDF(3);
        test_randomization(*rng, referenceDist, 0, 1, 1000, 0.01f);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_NegativelyBiasedRandomization_Bias1) {
        auto rng = createRandomizationBiasedToFalse(1);
        auto referenceDist = getReferenceECDF(-1);
        test_randomization(*rng, referenceDist, 0, 1, 1000, 0.01f);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_NegativelyBiasedRandomization_Bias3) {
        auto rng = createRandomizationBiasedToFalse(3);
        auto referenceDist = getReferenceECDF(-3);
        test_randomization(*rng, referenceDist, 0, 1, 1000, 0.01f);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_RandomizationCyclicallyBiasedToTrue_Bias1to7_step2_B1) {
        auto rng = createRandomizationCyclicallyBiasedToTrue(1, 7, 2);
        int measurementPeriod = 4;
        
        std::cout << "Testing bias 1... ";
        auto referenceDist = getReferenceECDF(1);
        int measurementStart = 0;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
        
        std::cout << "3... ";
        rng = createRandomizationCyclicallyBiasedToTrue(1, 7, 2);
        referenceDist = getReferenceECDF(3);
        measurementStart = 1;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
        
        std::cout << "5... ";
        rng = createRandomizationCyclicallyBiasedToTrue(1, 7, 2);
        referenceDist = getReferenceECDF(5);
        measurementStart = 2;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
        
        std::cout << "7... " << std::endl;
        rng = createRandomizationCyclicallyBiasedToTrue(1, 7, 2);
        referenceDist = getReferenceECDF(7);
        measurementStart = 3;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
    }
    
    TEST(RSRandomiztionTests, KolmogorovSmirnovTest_RandomizationCyclicallyBiasedToFalse_Bias1to7_step2_B1) {
        auto rng = createRandomizationCyclicallyBiasedToFalse(1, 7, 2);
        int measurementPeriod = 4;
        
        std::cout << "Testing bias 1... ";
        auto referenceDist = getReferenceECDF(-1);
        int measurementStart = 0;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
        
        std::cout << "3... ";
        rng = createRandomizationCyclicallyBiasedToFalse(1, 7, 2);
        referenceDist = getReferenceECDF(-3);
        measurementStart = 1;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
        
        std::cout << "5... ";
        rng = createRandomizationCyclicallyBiasedToFalse(1, 7, 2);
        referenceDist = getReferenceECDF(-5);
        measurementStart = 2;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
        
        std::cout << "7... " << std::endl;
        rng = createRandomizationCyclicallyBiasedToFalse(1, 7, 2);
        referenceDist = getReferenceECDF(-7);
        measurementStart = 3;
        test_randomization(*rng, referenceDist, measurementStart, measurementPeriod, 400, 0.01f);
    }
}
