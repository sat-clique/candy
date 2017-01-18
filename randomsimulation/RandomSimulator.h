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

#ifndef _E8029478_2FCD_4B07_9B47_58F51AA1C2FE_RANDOMSIMULATOR_H
#define _E8029478_2FCD_4B07_9B47_58F51AA1C2FE_RANDOMSIMULATOR_H

#include <memory>

#include "Conjectures.h"

class GateAnalyzer;

namespace randsim {
    class ClauseOrder;
    class Partition;
    class Randomization;
    class Propagation;
    
    class RandomSimulator {
    public:
        RandomSimulator();
        
        virtual Conjectures run(unsigned int nSteps) = 0;
        virtual Conjectures run() = 0;
        virtual ~RandomSimulator();
        
        RandomSimulator(const RandomSimulator &other) = delete;
        RandomSimulator& operator=(const RandomSimulator &other) = delete;
    };
    
    class RandomSimulatorBuilder {
    public:
        virtual RandomSimulatorBuilder& withClauseOrderStrategy(std::unique_ptr<ClauseOrder> clauseOrderStrat) = 0;
        virtual RandomSimulatorBuilder& withPartitionStrategy(std::unique_ptr<Partition> partitionStrat) = 0;
        virtual RandomSimulatorBuilder& withRandomizationStrategy(std::unique_ptr<Randomization> randomizationStrat) = 0;
        virtual RandomSimulatorBuilder& withPropagationStrategy(std::unique_ptr<Propagation> propagationStrat) = 0;
        virtual RandomSimulatorBuilder& withReductionRateAbortThreshold(float threshold) = 0;

        virtual RandomSimulatorBuilder& withGateAnalyzer(GateAnalyzer& gateAnalyzer) = 0;

        virtual std::unique_ptr<RandomSimulator> build() = 0;
        
        RandomSimulatorBuilder();
        virtual ~RandomSimulatorBuilder();
        RandomSimulatorBuilder(const RandomSimulatorBuilder& other) = delete;
        RandomSimulatorBuilder& operator=(const RandomSimulatorBuilder &other) = delete;
    };
    
    std::unique_ptr<RandomSimulator> createDefaultRandomSimulator();
    std::unique_ptr<RandomSimulatorBuilder> createDefaultRandomSimulatorBuilder();
}
#endif
