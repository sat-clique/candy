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

#ifndef X_948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H
#define X_948E36CB_0661_4DBC_9F3B_7F1C1DA546E0_PARTITION_H

#include <memory>
#include <vector>

#include <core/SolverTypes.h>

namespace Candy {
    class SimulationVectors;
    class Conjectures;
    
    /**
     * \class Partition
     *
     * \ingroup RandomSimulation
     *
     * \brief An object partitioning literals on grounds of equal assignments.
     *
     * Partition objects are responsible for maintaining partitions of literals such
     * that literals remain in a shared partition if and only if they had the same
     * value for each variable assignment passed to the Partition object. The partitions
     * can be retrieved as equivalence/backbone conjectures.
     *
     * Usage example: build a RandomSimulation object with this strategy.
     */
    class Partition {
    public:
        /**
         * Sets the variables for whose literals need to be partitioned.
         */
        virtual void setVariables(const std::vector<Glucose::Var> &variables) = 0;
        
        /**
         * Updates the partitions using the given variable assignment.
         */
        virtual void update(const SimulationVectors &assignment) = 0;
        
        /**
         * Retrieves the current partitioning in form of conjectures. Note that
         * partitions containing a single, non-backbone literal are omitted from
         * the results.
         */
        virtual Conjectures getConjectures() = 0;
        
        /**
         * Gets the current RRAT, which is computed when the partition gets compressed.
         */
        virtual float getPartitionReductionRate() = 0;
        
        Partition();
        virtual ~Partition();
        Partition(const Partition& other) = delete;
        Partition& operator=(const Partition &other) = delete;
    };
    
    
    /**
     * \class CompressionScheduleStrategy
     *
     * \ingroup RandomSimulation
     *
     * \brief A strategy determining the points of time on which a partition strategy
     *   should optimize its memory usage.
     *
     * Usage example: build a RandomSimulation object with this strategy.
     */
    class CompressionScheduleStrategy {
    public:
        /**
         * Returns true iff compression should be performed at the round m_round.
         */
        virtual bool shouldCompress(unsigned int m_round) = 0;
        
        CompressionScheduleStrategy();
        virtual ~CompressionScheduleStrategy();
        CompressionScheduleStrategy(const CompressionScheduleStrategy& other) = delete;
        CompressionScheduleStrategy& operator=(const CompressionScheduleStrategy &other) = delete;
    };
    
    /**
     * Creates a Partition object using a logarithmic compression schedule strategy (i.e. compression
     * at every 2^Nth step).
     */
    std::unique_ptr<Partition> createDefaultPartition();
    
    /**
     * Creates a Partition object using a custom compression schedule.
     */
    std::unique_ptr<Partition> createDefaultPartition(std::unique_ptr<CompressionScheduleStrategy> compressionScheduleStrategy);
    
    /**
     * Creates a linear compression schedule.
     *
     * \param freq The compression frequency: compress every freq steps
     */
    std::unique_ptr<CompressionScheduleStrategy> createLinearCompressionScheduleStrategy(unsigned int freq);
    
    /**
     * Creates a logarithmic compression schedule (i.e. compression
     * at every 2^Nth step).
     */
    std::unique_ptr<CompressionScheduleStrategy> createLogCompressionScheduleStrategy();
    
    /**
     * Creates a "null" compression schedule, i.e. no compression at all.
     */
    std::unique_ptr<CompressionScheduleStrategy> createNullCompressionScheduleStrategy();
}

#endif
