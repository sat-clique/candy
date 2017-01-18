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

#ifndef _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H
#define _948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H

#include <memory>
#include <vector>

#include <core/SolverTypes.h>

namespace randsim {
    class SimulationVectors;
    class Conjectures;
    
    class Partition {
    public:
        virtual void setVariables(const std::vector<Glucose::Var> &variables) = 0;
        virtual void update(const SimulationVectors &assignment) = 0;
        virtual Conjectures getConjectures() = 0;
        virtual float getPartitionReductionRate() = 0;
        
        Partition();
        virtual ~Partition();
        Partition(const Partition& other) = delete;
        Partition& operator=(const Partition &other) = delete;
    };
    
    class CompressionScheduleStrategy {
    public:
        virtual bool shouldCompress(unsigned int m_round) = 0;
        
        CompressionScheduleStrategy();
        virtual ~CompressionScheduleStrategy();
        CompressionScheduleStrategy(const CompressionScheduleStrategy& other) = delete;
        CompressionScheduleStrategy& operator=(const CompressionScheduleStrategy &other) = delete;
    };
    
    std::unique_ptr<Partition> createDefaultPartition();
    std::unique_ptr<Partition> createDefaultPartition(std::unique_ptr<CompressionScheduleStrategy> compressionScheduleStrategy);
    
    std::unique_ptr<CompressionScheduleStrategy> createLinearCompressionScheduleStrategy(unsigned int freq);
    std::unique_ptr<CompressionScheduleStrategy> createLogCompressionScheduleStrategy();
    std::unique_ptr<CompressionScheduleStrategy> createNullCompressionScheduleStrategy();
}

#endif
