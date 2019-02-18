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

// TODO: documentation

#include "Partition.h"

#include <memory>
#include <unordered_map>
#include <cstdint>

#include <candy/utils/FastRand.h>
#include <candy/utils/MemUtils.h>

#include "SimulationVector.h"
#include "Conjectures.h"

#define FASTVARIABLEPARTITIONING_GID_SIZE_BITS sizeof(SimulationVector::varsimvec_field_t)*8
#define FASTVARIABLEPARTITIONING_VARID_SIZE_BITS 28

namespace Candy {
    Partition::Partition() {
        
    }
    
    Partition::~Partition() {
        
    }
    
    
    
    CompressionScheduleStrategy::CompressionScheduleStrategy() {
        
    }
    
    CompressionScheduleStrategy::~CompressionScheduleStrategy() {
        
    }
    
    
    class LinearCompressionScheduleStrategy : public CompressionScheduleStrategy {
    public:
        bool shouldCompress(unsigned int round) override;
        
        explicit LinearCompressionScheduleStrategy(unsigned int freq);
        virtual ~LinearCompressionScheduleStrategy();
        LinearCompressionScheduleStrategy(const LinearCompressionScheduleStrategy& other) = delete;
        LinearCompressionScheduleStrategy& operator=(const LinearCompressionScheduleStrategy &other) = delete;
        
    private:
        unsigned int m_freq;
    };
    
    LinearCompressionScheduleStrategy::LinearCompressionScheduleStrategy(unsigned int freq)
    : CompressionScheduleStrategy(), m_freq(freq) {
        
    }
    
    LinearCompressionScheduleStrategy::~LinearCompressionScheduleStrategy() {
        
    }
    
    bool LinearCompressionScheduleStrategy::shouldCompress(unsigned int round) {
        return round % m_freq == 0;
    }
    
    
    
    class LogCompressionScheduleStrategy : public CompressionScheduleStrategy {
    public:
        bool shouldCompress(unsigned int round) override;
        
        LogCompressionScheduleStrategy();
        virtual ~LogCompressionScheduleStrategy();
        LogCompressionScheduleStrategy(const LinearCompressionScheduleStrategy& other) = delete;
        LinearCompressionScheduleStrategy& operator=(const LinearCompressionScheduleStrategy &other) = delete;
    };
    
    LogCompressionScheduleStrategy::LogCompressionScheduleStrategy()
    : CompressionScheduleStrategy() {
        
    }
    
    LogCompressionScheduleStrategy::~LogCompressionScheduleStrategy() {
        
    }
    
    bool LogCompressionScheduleStrategy::shouldCompress(unsigned int round) {
        return round == 0 || !(round & (round - 1));
    }
    
    
    
    class NullCompressionScheduleStrategy : public CompressionScheduleStrategy {
    public:
        bool shouldCompress(unsigned int round) override;
        
        NullCompressionScheduleStrategy();
        virtual ~NullCompressionScheduleStrategy();
        NullCompressionScheduleStrategy(const NullCompressionScheduleStrategy& other) = delete;
        NullCompressionScheduleStrategy& operator=(const NullCompressionScheduleStrategy &other) = delete;
    };
    
    NullCompressionScheduleStrategy::NullCompressionScheduleStrategy()
    : CompressionScheduleStrategy() {
        
    }
    
    NullCompressionScheduleStrategy::~NullCompressionScheduleStrategy() {
        
    }
    
    bool NullCompressionScheduleStrategy::shouldCompress(unsigned int) {
        return false;
    }
    
    
    
    std::unique_ptr<CompressionScheduleStrategy> createLinearCompressionScheduleStrategy(unsigned int freq) {
        return backported_std::make_unique<LinearCompressionScheduleStrategy>(freq);
    }
    
    std::unique_ptr<CompressionScheduleStrategy> createLogCompressionScheduleStrategy() {
        return backported_std::make_unique<LogCompressionScheduleStrategy>();
    }
    
    std::unique_ptr<CompressionScheduleStrategy> createNullCompressionScheduleStrategy() {
        return backported_std::make_unique<NullCompressionScheduleStrategy>();
    }
    
    
    
    
    
    class DefaultPartition : public Partition {
    public:
        explicit DefaultPartition(std::unique_ptr<CompressionScheduleStrategy> compressionSched);
        
        void setVariables(const std::vector<Candy::Var> &variables) override;
        void update(const SimulationVectors &assignment) override;
        Conjectures getConjectures() override;
        float getPartitionReductionRate() override;
        
        virtual ~DefaultPartition();
        DefaultPartition(const DefaultPartition& other) = delete;
        DefaultPartition& operator=(const DefaultPartition &other) = delete;
        
    private:
        class PartitionEntry {
        public:
            uint64_t groupId : FASTVARIABLEPARTITIONING_GID_SIZE_BITS;
            uint64_t varId : FASTVARIABLEPARTITIONING_VARID_SIZE_BITS;
            uint64_t backbone : 1;
            uint64_t backboneVal : 1;
            uint64_t polarity : 1;
        };
        
        /* update-related functions */
        void ensureInitialized(const SimulationVectors &assignment);
        void initializePartitionEntry(size_t index, const SimulationVectors &varAssignments);
        void updatePartitioningPos(const SimulationVectors &varAssignments, bool isInitialUpdate);
        void updatePartitioningNeg(const SimulationVectors &varAssignments, bool isInitialUpdate);
        
        
        /* compression-related functions */
        void writeBackPartitionEntry(std::vector<PartitionEntry> &partition,
                                     std::unordered_map<uint64_t, uint64_t> &varIdToNegGroupId,
                                     int &rewriteIndex);
        void getPartitionsByID(std::unordered_map<uint64_t, std::vector<PartitionEntry>>
                               &partitionsByID,
                               std::vector<uint64_t> &groupIDs_detOrder,
                               std::unordered_map<uint64_t, uint64_t> *varIdToNegGroupId);
        void compressPartitioning();
        
        /* conjecture-getting-related functions */
        bool isIgnoringPartition(std::vector<PartitionEntry> &partition,
                                 std::vector<char> &seenVariables);
        void addToConjectures(std::vector<PartitionEntry> &partition,
                              Conjectures &target);
        void addBackbonesToConjectures(Conjectures &target);
        
        unsigned int m_round;
        std::vector<PartitionEntry> m_partitioningPos;
        std::vector<PartitionEntry> m_partitioningNeg;
        std::vector<Candy::Var> m_variables;
        
        std::unique_ptr<CompressionScheduleStrategy> m_compressionSched;
        std::uint64_t m_correlationCount;
        
        float m_reductionRate = 1.0f;
    };
    
    DefaultPartition::DefaultPartition(std::unique_ptr<CompressionScheduleStrategy> compressionSched)
    : Partition(),
    m_round(0),
    m_partitioningPos({}),
    m_partitioningNeg({}),
    m_variables({}),
    m_compressionSched(std::move(compressionSched)),
    m_correlationCount(0) {
        
    }
    
    DefaultPartition::~DefaultPartition() {
        
    }
    
    void DefaultPartition::setVariables(const std::vector<Candy::Var> &variables) {
        m_variables = variables;
    }
    
    void DefaultPartition::initializePartitionEntry(size_t index, const SimulationVectors &varAssignments) {
        
        PartitionEntry partitionEntry;
        partitionEntry.groupId = fastNextRand(varAssignments.getConst(index).vars[0]);
        partitionEntry.varId = index;
        partitionEntry.backbone = 1;
        partitionEntry.polarity = 1;
        partitionEntry.backboneVal = (varAssignments.getConst(index).vars[0] & 1);
        
        m_partitioningPos.push_back(partitionEntry);
        
        partitionEntry.backbone = 0;
        partitionEntry.polarity = 0;
        partitionEntry.groupId =
          fastNextRand(~(varAssignments.getConst(index).vars[0]));
        m_partitioningNeg.push_back(partitionEntry);
    }
    
    void DefaultPartition::updatePartitioningPos(const SimulationVectors &varAssignments, bool isInitialUpdate) {
        for (size_t peIdx = 0; peIdx < m_partitioningPos.size(); ++peIdx) {
            PartitionEntry &partitionEntry = m_partitioningPos[peIdx];
            bool bbBefore = partitionEntry.backbone;
            (void)bbBefore; // prevents release-mode compiler warning about unused variable (see assertion below)
            auto &simVec = varAssignments.getConst(partitionEntry.varId);
            
            for (size_t svIdx = (isInitialUpdate ? 1 : 0); svIdx < SimulationVector::VARSIMVECSIZE;
                 ++svIdx) {
                partitionEntry.groupId =
                  fastNextRand(partitionEntry.groupId ^ simVec.vars[svIdx]);
            }
            
            if (partitionEntry.backbone) {
                for (size_t svIdx = 0; svIdx < SimulationVector::VARSIMVECSIZE; ++svIdx) {
                    SimulationVector::varsimvec_field_t svItem = simVec.vars[svIdx];
                    partitionEntry.backbone &=
                      (partitionEntry.backboneVal ? ~svItem == 0ull : svItem == 0ull);
                }
            }
            
            assert(!partitionEntry.backbone || bbBefore);
        }
    }
    
    void DefaultPartition::updatePartitioningNeg(const SimulationVectors &varAssignments, bool isInitialUpdate) {
        for (size_t peIdx = 0; peIdx < m_partitioningPos.size(); ++peIdx) {
            PartitionEntry &partitionEntry = m_partitioningNeg[peIdx];
            
            auto &simVec = varAssignments.getConst(partitionEntry.varId);
            
            for (size_t svIdx = (isInitialUpdate ? 1 : 0); svIdx < SimulationVector::VARSIMVECSIZE;
                 ++svIdx) {
                partitionEntry.groupId =
                  fastNextRand(partitionEntry.groupId ^ ~(simVec.vars[svIdx]));
            }
        }
    }
    
    void DefaultPartition::ensureInitialized(const SimulationVectors &assignment) {
        if (m_round != 0) {
            return;
        }
        
       
        for (Candy::Var variable : m_variables) {
            initializePartitionEntry(variable, assignment);
        }
        
        updatePartitioningPos(assignment, true);
        updatePartitioningNeg(assignment, true);
    }
    
    void DefaultPartition::update(const SimulationVectors &assignment) {
        ensureInitialized(assignment);
        
        updatePartitioningPos(assignment, false);
        updatePartitioningNeg(assignment, false);
        
        if (m_compressionSched->shouldCompress(m_round)) {
            auto prevCorrCount = m_correlationCount;
            compressPartitioning();
            
            if (prevCorrCount != 0) {
                m_reductionRate = 1.0f - ((float)m_correlationCount / (float)prevCorrCount);
            }
        }
        
        ++m_round;
    }
    
    void DefaultPartition::writeBackPartitionEntry(std::vector<PartitionEntry> &partition,
                                                   std::unordered_map<uint64_t, uint64_t> &varIdToNegGroupId,
                                                   int &rewriteIndex) {
        assert(partition.size() >= 1);
        assert(partition.size() > 1 || partition[0].backbone);
        
        bool allPositivePolarity = true;
        bool allNegativePolarity = true;

        for (auto &partitionEntry : partition) {
            allPositivePolarity &= partitionEntry.polarity;
            allNegativePolarity &= ~(partitionEntry.polarity);
            
            if (partitionEntry.polarity == 1) {
                m_partitioningPos[rewriteIndex] = partitionEntry;
                
                PartitionEntry peNeg;
                peNeg.varId = partitionEntry.varId;
                peNeg.groupId = varIdToNegGroupId[partitionEntry.varId];
                peNeg.polarity = 0;
                peNeg.backbone = 0;
                peNeg.backboneVal = 0;
                m_partitioningNeg[rewriteIndex] = peNeg;
                
                ++rewriteIndex;
            }
        }
        
        // actually computing 2*m_correlationCount, which is okay for computing the reduction rate
        // because the factor 2 is canceled out.
        std::uint64_t partitionSize = partition.size();
        if (partition[0].backbone) {
            m_correlationCount += 2 * partitionSize;
        }
        else if (!allNegativePolarity && !allPositivePolarity) {
            // A mirroring partition exists for which this condition is true as well
            m_correlationCount += ((partitionSize * partitionSize) - partitionSize)/2;
        }
        else if (allPositivePolarity) {
            m_correlationCount += ((partitionSize * partitionSize) - partitionSize);
        }
    }

    void DefaultPartition::getPartitionsByID(std::unordered_map<uint64_t, std::vector<PartitionEntry>>
                                                 &partitionsByID,
                                             std::vector<uint64_t> &groupIDs_detOrder,
                                             std::unordered_map<uint64_t, uint64_t> *varIdToNegGroupId) {
        for (size_t i = 0; i < m_partitioningPos.size(); i++) {
            PartitionEntry &pePos = m_partitioningPos[i];
            PartitionEntry &peNeg = m_partitioningNeg[i];
            
            if (partitionsByID[pePos.groupId].size() == 0) {
                groupIDs_detOrder.push_back(pePos.groupId);
            }
            if (partitionsByID[peNeg.groupId].size() == 0) {
                groupIDs_detOrder.push_back(peNeg.groupId);
            }
            
            partitionsByID[pePos.groupId].push_back(pePos);
            partitionsByID[peNeg.groupId].push_back(peNeg);
            if (varIdToNegGroupId) {
                (*varIdToNegGroupId)[pePos.varId] = peNeg.groupId;
            }
        }
    }

    
    void DefaultPartition::compressPartitioning() {
        std::unordered_map<std::uint64_t, std::vector<PartitionEntry>> partitions;
        m_correlationCount = 0;
        std::vector<uint64_t> groupIDs_detOrder;
        std::unordered_map<uint64_t, uint64_t> varIdToNegGroupId;
        
        getPartitionsByID(partitions, groupIDs_detOrder, &varIdToNegGroupId);
        
        int rewriteIdx = 0;
        for (uint64_t posGroupID : groupIDs_detOrder) {
            auto &partition = partitions[posGroupID];
            auto partitionSize = partition.size();
            
            assert (partitionSize >= 1);
                
            if (partitionSize > 1 || (partition[0].polarity == 1 && partition[0].backbone)) {
                writeBackPartitionEntry(partition, varIdToNegGroupId, rewriteIdx);
            }
            
            
        }
        
        m_partitioningPos.resize(rewriteIdx);
        m_partitioningNeg.resize(rewriteIdx);
    }
    
    bool DefaultPartition::isIgnoringPartition(std::vector<PartitionEntry> &partition,
                                               std::vector<char> &seenVariables) {
        /* Keep only partitions having at least one positive-polarity
         PartitionEntry. (All-negatives are considered irrelevant
         due to the presence of a mirrored all-positive partition.)
         If the algorithm has already started adding the mirrored
         partition, drop the current one. (Backbone PEs are never of
         negative polarity.) */
        bool hasPosPEForSeenVar = false;
        bool hasPosPE = false;
        bool hasBackbone = false;
        for (PartitionEntry &pe : partition) {
            hasPosPEForSeenVar |= (pe.polarity == 0 && seenVariables[pe.varId]);
            hasPosPE |= pe.polarity;
            seenVariables[pe.varId] = seenVariables[pe.varId] | pe.polarity;
            hasBackbone |= pe.backbone;
        }
        if (hasPosPEForSeenVar || !hasPosPE || hasBackbone) {
            return true;
        }
        
        return false;
    }
    
    void DefaultPartition::addToConjectures(std::vector<PartitionEntry> &partition,
                                            Conjectures &target) {
        if (partition.size() > 1) {
            EquivalenceConjecture conj;
            for (auto &partitionEntry : partition) {
                /* TODO: del? if (!isVariableEnabled(partitionEntry.varId)) {
                    continue;
                }*/
                if (!partitionEntry.backbone) {
                    conj.addLit(mkLit(partitionEntry.varId, partitionEntry.polarity));
                }
                // TODO: del? setIsSweepingArtifactVariable(partitionEntry.varId, true);
            }
            if (conj.size() >= 2) {
                target.addEquivalence(conj);
            }
        }
    }
    
    void DefaultPartition::addBackbonesToConjectures(Conjectures &target) {
        for (auto &partitionEntry : m_partitioningPos) {
            if (partitionEntry.backbone) {
                BackboneConjecture conj {mkLit(partitionEntry.varId, partitionEntry.backboneVal)};
                target.addBackbone(conj);
            }
        }
    }


    Conjectures DefaultPartition::getConjectures() {
        std::unordered_map<uint64_t, std::vector<PartitionEntry>> partitions;
        std::vector<uint64_t> groupIDs_detOrder;
        
        getPartitionsByID(partitions, groupIDs_detOrder, nullptr);
        
        Candy::Var maxVar = 0;
        for (auto var : m_variables) {
            if (var > maxVar) {
                maxVar = var;
            }
        }
        
        std::vector<char> seenVariables;
        seenVariables.resize(maxVar+1);
        
        Conjectures result;
        addBackbonesToConjectures(result);
        for (uint64_t posGroupID : groupIDs_detOrder) {
            auto &partition = partitions[posGroupID];
            if (!isIgnoringPartition(partition, seenVariables)) {
                addToConjectures(partition, result);
            }
        }
        
        return result;
    }
    
    float DefaultPartition::getPartitionReductionRate() {
        return m_reductionRate;
    }
    
    std::unique_ptr<Partition> createDefaultPartition() {
        return createDefaultPartition(backported_std::make_unique<LogCompressionScheduleStrategy>());
    }
    
    std::unique_ptr<Partition> createDefaultPartition(std::unique_ptr<CompressionScheduleStrategy> compressionScheduleStrategy) {
        return backported_std::make_unique<DefaultPartition>(std::move(compressionScheduleStrategy));
    }
}
