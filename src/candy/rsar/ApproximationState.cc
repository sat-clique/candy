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

#include "ApproximationState.h"
#include <randomsimulation/Conjectures.h>
#include <utils/MemUtils.h>

#include <set>
#include <unordered_map>
#include <memory>
#include <deque>
#include <vector>
#include <utility>
#include <map>

#include <iostream>

namespace Candy {
    
    /* -- ApproximationDelta implementation --------------------------------------------------------- */
    
    ApproximationDelta::ApproximationDelta() noexcept {
        
    }
    
    ApproximationDelta::~ApproximationDelta() {
        
    }
    
    class ApproximationDeltaImpl : public ApproximationDelta {
    public:
        explicit ApproximationDeltaImpl(const std::vector<EquivalenceImplications::CommitResult>& eqCommitRes,
                                        const Backbones::CommitResult& bbComitRes) noexcept;
        
        const_implication_iterator beginRemovedImplications() const noexcept override;
        const_implication_iterator endRemovedImplications() const noexcept override;
        const_implication_size_type removedImplicationsSize() const noexcept override;
        
        const_implication_iterator beginAddedImplications() const noexcept override;
        const_implication_iterator endAddedImplications() const noexcept override;
        const_implication_size_type addedImplicationsSize() const noexcept override;
        
        const_backbone_iterator beginRemovedBackbones() const noexcept override;
        const_backbone_iterator endRemovedBackbones() const noexcept override;
        const_backbone_size_type removedBackbonesSize() const noexcept override;
        
        virtual ~ApproximationDeltaImpl();
        ApproximationDeltaImpl(const ApproximationDeltaImpl &other) = delete;
        ApproximationDeltaImpl& operator= (const ApproximationDeltaImpl& other) = delete;
        
    private:
        std::vector<Implication> m_addedImplications;
        std::vector<Implication> m_removedImplications;
        std::vector<Lit> m_removedBackbones;
    };
     
    
    ApproximationDeltaImpl::ApproximationDeltaImpl(const std::vector<EquivalenceImplications::CommitResult>& eqCommitRes,
                                     const Backbones::CommitResult& bbCommitRes) noexcept
    : ApproximationDelta(),
    m_addedImplications(),
    m_removedImplications(),
    m_removedBackbones(bbCommitRes.removedBackbones) {
        for (auto&& eqCommit : eqCommitRes) {
            m_addedImplications.insert(m_addedImplications.end(),
                                       eqCommit.newImplications.begin(),
                                       eqCommit.newImplications.end());
            m_removedImplications.insert(m_removedImplications.end(),
                                         eqCommit.removedImplications.begin(),
                                         eqCommit.removedImplications.end());
        }
    }
    
    ApproximationDelta::const_implication_iterator  ApproximationDeltaImpl::beginAddedImplications() const noexcept{
        return m_addedImplications.begin();
    }
    
    ApproximationDelta::const_implication_iterator ApproximationDeltaImpl::endAddedImplications() const noexcept {
        return m_addedImplications.end();
    }
    
    ApproximationDelta::const_implication_size_type ApproximationDeltaImpl::addedImplicationsSize() const noexcept {
        return m_addedImplications.size();
    }
    
    ApproximationDelta::const_implication_iterator ApproximationDeltaImpl::beginRemovedImplications() const noexcept {
        return m_removedImplications.begin();
    }
    
    ApproximationDelta::const_implication_iterator ApproximationDeltaImpl::endRemovedImplications() const noexcept {
        return m_removedImplications.end();
    }
    
    ApproximationDelta::const_implication_size_type ApproximationDeltaImpl::removedImplicationsSize() const noexcept {
        return m_removedImplications.size();
    }
    
    ApproximationDelta::const_backbone_iterator ApproximationDeltaImpl::beginRemovedBackbones() const noexcept {
        return m_removedBackbones.begin();
    }
    
    ApproximationDelta::const_backbone_iterator ApproximationDeltaImpl::endRemovedBackbones() const noexcept {
        return m_removedBackbones.end();
    }
    
    ApproximationDelta::const_backbone_size_type ApproximationDeltaImpl::removedBackbonesSize() const noexcept {
        return m_removedBackbones.size();
    }
    
    ApproximationDeltaImpl::~ApproximationDeltaImpl() {
        
    }
    
    
    std::unique_ptr<ApproximationDelta> createApproximationDelta(const std::vector<EquivalenceImplications::CommitResult>& eqCommitRes,
                                                                 const Backbones::CommitResult& bbComitRes) noexcept {
        return backported_std::make_unique<ApproximationDeltaImpl>(eqCommitRes, bbComitRes);
    }
    
    
    /* -- Backbones implementation --------------------------------------------------------- */
    
    Backbones::Backbones() noexcept {
        
    }
    
    Backbones::~Backbones() {
        
    }
    
    class BackbonesImpl : public Backbones {
    public:
        explicit BackbonesImpl(const std::vector<BackboneConjecture>& conjectures) noexcept;
        
        void addVariableRemovalToWorkQueue(Var it) noexcept override;
        
        const_iterator begin() const noexcept override;
        const_iterator end() const noexcept override;
        size_type size() const noexcept override;
        bool empty() const noexcept override;
        Lit at(size_type index) const override;
        
        const Backbones::CommitResult commitWorkQueue();
        void commitWorkQueueWithoutDelta();
        
        
        virtual ~BackbonesImpl();
        BackbonesImpl(const BackbonesImpl& other) = delete;
        BackbonesImpl& operator=(const BackbonesImpl& other) = delete;
        
    private:
        /** A vector representation of m_backbones, synced with m_backbones
         * when committing changes. */
        std::vector<Lit> m_backbonesCached;
        
        /** The set of represented backbone literals. */
        std::set<Lit> m_backbones;
        
        /** The variables to be removed. */
        std::set<Var> m_removalWorkQueue;
        
        friend const Backbones::CommitResult test_commitWorkQueue(Backbones& target);
        friend void test_commitWorkQueueWithoutDelta(Backbones& target);
    };
    
    
    BackbonesImpl::BackbonesImpl(const std::vector<BackboneConjecture>& conjectures) noexcept
    : Backbones(),
    m_backbonesCached(),
    m_backbones(),
    m_removalWorkQueue() {
        for (auto& bbConj : conjectures) {
            m_backbonesCached.push_back(bbConj.getLit());
            m_backbones.insert(bbConj.getLit());
        }
    }
    
    void BackbonesImpl::addVariableRemovalToWorkQueue(Var removal) noexcept {
        m_removalWorkQueue.insert(removal);
    }
    
    BackbonesImpl::const_iterator BackbonesImpl::begin() const noexcept {
        return m_backbonesCached.begin();
    }
    
    BackbonesImpl::const_iterator BackbonesImpl::end() const noexcept {
        return m_backbonesCached.end();
    }
    
    BackbonesImpl::size_type BackbonesImpl::size() const noexcept {
        return m_backbonesCached.size();
    }
    
    bool BackbonesImpl::empty() const noexcept {
        return m_backbonesCached.empty();
    }
    
    Lit BackbonesImpl::at(size_type index) const {
        return m_backbonesCached[index];
    }
    
    
    const Backbones::CommitResult BackbonesImpl::commitWorkQueue() {
        Backbones::CommitResult result;
        
        for (Var v : m_removalWorkQueue) {
            Lit candidate = mkLit(v, 0);
            
            if (m_backbones.find(candidate) == m_backbones.end()) {
                candidate = ~candidate;
            }
            
            if (m_backbones.find(candidate) != m_backbones.end()) {
                m_backbones.erase(candidate);
                result.removedBackbones.push_back(candidate);
            }
        }
        
        m_removalWorkQueue.clear();
        
        m_backbonesCached.clear();
        for (Lit l : m_backbones) {
            m_backbonesCached.push_back(l);
        }
        
        return result;
    }
    
    void BackbonesImpl::commitWorkQueueWithoutDelta() {
        commitWorkQueue();
    }
    
    std::unique_ptr<BackbonesImpl> createBackbones(const std::vector<BackboneConjecture>& conjectures) {
        return backported_std::make_unique<BackbonesImpl>(conjectures);
    }
    
    BackbonesImpl::~BackbonesImpl() {
        
    }
    
    const Backbones::CommitResult test_commitWorkQueue(Backbones& target) {
#ifndef CANDY_HAS_NO_RTTI
        BackbonesImpl* impl = dynamic_cast<BackbonesImpl*>(&target);
        assert(impl != nullptr);
#else
        BackbonesImpl* impl = reinterpret_cast<BackbonesImpl*>(&target);
#endif
        return impl->commitWorkQueue();
    }
    
    void test_commitWorkQueueWithoutDelta(Backbones& target) {
#ifndef CANDY_HAS_NO_RTTI
        BackbonesImpl* impl = dynamic_cast<BackbonesImpl*>(&target);
        assert(impl != nullptr);
#else
        BackbonesImpl* impl = reinterpret_cast<BackbonesImpl*>(&target);
#endif
        impl->commitWorkQueue();
    }
    
    
    /* -- EquivalenceImplications implementation --------------------------------------------------------- */
    
    EquivalenceImplications::EquivalenceImplications() noexcept {
        
    }
    
    EquivalenceImplications::~EquivalenceImplications() {
        
    }
    
    class EquivalenceImplicationsImpl : public EquivalenceImplications {
    public:
        /**
         * Constructs a new EquivalenceImplicationsImpl object using the 
         * given equivalence conjecture. Note that if the conjecture
         * contains fewer than two literals, an empty EquivalenceImplicationsImpl
         * object is constructed.
         */
        explicit EquivalenceImplicationsImpl(const EquivalenceConjecture &conjecture) noexcept;
        
        void addVariableRemovalToWorkQueue(Var it) noexcept override;
        
        const EquivalenceImplications::CommitResult commitWorkQueue();
        void commitWorkQueueWithoutDelta();
        
        const_iterator begin() const noexcept override;
        const_iterator end() const noexcept override;
        size_type size() const noexcept override;
        bool empty() const noexcept override;
        Implication at(size_type index) const override;
        
        virtual ~EquivalenceImplicationsImpl();
        EquivalenceImplicationsImpl(const EquivalenceImplicationsImpl& other) = delete;
        EquivalenceImplicationsImpl& operator=(const EquivalenceImplicationsImpl& other) = delete;
        
    private:
        /**
         * Registers the given implication with m_implicationsByAnte and m_implicationsBySucc.
         */
        void addImplication(Implication implication);
        
        /**
         * Removes the given implication from m_implicationsByAnte and m_implicationsBySucc.
         */
        void removeImplication(Implication implication);
        
        /**
         * Clears m_varRemovalWorkQueue and populates it with the value set of m_implicationsByAnte.
         */
        void updateCache();
        
        /** Maps variables V to the implication A->B where var(A) = V. */
        std::map<Var, Implication> m_implicationsByAnte;
        
        /** Maps variables V to the implication A->B where var(B) = V. */
        std::map<Var, Implication> m_implicationsBySucc;
        
        /** The cached set of implications. */
        std::vector<Implication> m_implicationsCached;
        
        /** The variables to be removed. */
        std::set<Var> m_varRemovalWorkQueue;
        
        friend const EquivalenceImplications::CommitResult test_commitWorkQueue(EquivalenceImplications& target);
        friend void test_commitWorkQueueWithoutDelta(EquivalenceImplications& target);
    };
    
    
    EquivalenceImplicationsImpl::EquivalenceImplicationsImpl(const EquivalenceConjecture &conjecture) noexcept
    : EquivalenceImplications(),
    m_implicationsByAnte(),
    m_implicationsBySucc(),
    m_implicationsCached(),
    m_varRemovalWorkQueue() {
        
        //auto& vars = conjecture.getLits();
        
        if (conjecture.size() > 1) {
            for (size_t i = 0; i < conjecture.size()-1; ++i) {
                addImplication(Implication{conjecture.at(i), conjecture.at(i+1)});
            }
            addImplication(Implication{conjecture.at(conjecture.size()-1), conjecture.at(0)});
        }
        updateCache();
    }
    
    void EquivalenceImplicationsImpl::addImplication(Implication implication) {
        m_implicationsByAnte[var(implication.first)] = implication;
        m_implicationsBySucc[var(implication.second)] = implication;
    }
    
    void EquivalenceImplicationsImpl::removeImplication(Implication implication) {
        m_implicationsByAnte.erase(var(implication.first));
        m_implicationsBySucc.erase(var(implication.second));
    }
    
    void EquivalenceImplicationsImpl::updateCache() {
        m_implicationsCached.clear();
        for (auto& varAndImpl : m_implicationsByAnte) {
            m_implicationsCached.push_back(varAndImpl.second);
        }
    }
    
    void EquivalenceImplicationsImpl::addVariableRemovalToWorkQueue(Var toBeRemoved) noexcept {
        m_varRemovalWorkQueue.insert(toBeRemoved);
    }
    
    EquivalenceImplications::const_iterator EquivalenceImplicationsImpl::begin() const noexcept {
        return m_implicationsCached.begin();
    }
    
    EquivalenceImplications::const_iterator EquivalenceImplicationsImpl::end() const noexcept {
        return m_implicationsCached.end();
    }
    
    EquivalenceImplications::size_type EquivalenceImplicationsImpl::size() const noexcept {
        return m_implicationsCached.size();
    }
    
    bool EquivalenceImplicationsImpl::empty() const noexcept {
        return m_implicationsCached.empty();
    }
    
    Implication EquivalenceImplicationsImpl::at(size_type index) const {
        return m_implicationsCached[index];
    }
    
    const EquivalenceImplications::CommitResult EquivalenceImplicationsImpl::commitWorkQueue() {
        EquivalenceImplications::CommitResult result;
        
        if (m_varRemovalWorkQueue.empty()) {
            return result;
        }
        
        
        while (!m_varRemovalWorkQueue.empty()) {
            Var first = *m_varRemovalWorkQueue.begin();
            
            // if we don't have any implications using the variable, ignore
            if (m_implicationsByAnte.find(first) == m_implicationsByAnte.end()) {
                // TODO: maybe emit a warning?
                m_varRemovalWorkQueue.erase(first);
                continue;
            }
            
            std::deque<Implication> removalQueue;
            removalQueue.insert(removalQueue.begin(), m_implicationsBySucc[first]);
            removalQueue.insert(removalQueue.end(), m_implicationsByAnte[first]);
            
            
            // collect longest sequence of implications to be removed (forward part)
            for (Var current = var(m_implicationsByAnte[first].second);
                 current != first
                 && m_varRemovalWorkQueue.find(current) != m_varRemovalWorkQueue.end()
                 && removalQueue.front().first != removalQueue.back().second;
                 current = var(m_implicationsByAnte[current].second)) {
                
                removalQueue.push_back(m_implicationsByAnte[current]);
            }

            // collect longest sequence of implications to be removed (backward part)
            for (Var current = var(m_implicationsBySucc[first].first);
                 current != first && m_varRemovalWorkQueue.find(current) != m_varRemovalWorkQueue.end()
                 && removalQueue.front().first != removalQueue.back().second;
                 current = var(m_implicationsBySucc[current].first)) {
                removalQueue.push_front(m_implicationsBySucc[current]);
            }

            
            // remove implications
            //std::cout << "Removing " << removalQueue.size() << " implications" << std::endl;
            for (auto impl : removalQueue) {
                removeImplication(impl);
            }
            
            // insert new implication if appropriate
            Lit newAnte = removalQueue.front().first;
            Lit newSucc = removalQueue.back().second;
            
            //std::cout << "newAnte: " << newAnte << " newSucc: " << newSucc << std::endl;
            
            if (newAnte != newSucc
                && m_varRemovalWorkQueue.find(var(newAnte)) == m_varRemovalWorkQueue.end()
                && m_varRemovalWorkQueue.find(var(newSucc)) == m_varRemovalWorkQueue.end()) {
                // create patch X->Y
                //std::cout << "Adding a patch" << std::endl;
                addImplication(Implication{newAnte, newSucc});
                result.newImplications.push_back(Implication{newAnte, newSucc});
            }
            
            // update removal queue
            for (auto impl : removalQueue) {
                m_varRemovalWorkQueue.erase(var(impl.first));
                m_varRemovalWorkQueue.erase(var(impl.second));
            }
            
            result.removedImplications.insert(result.removedImplications.end(),
                                              removalQueue.begin(), removalQueue.end());
        }
        updateCache();
        return result;
    }
    
    void EquivalenceImplicationsImpl::commitWorkQueueWithoutDelta() {
        commitWorkQueue(); // TODO: optimize: don't generate the delta
    }

    std::unique_ptr<EquivalenceImplicationsImpl> createEquivalenceImplications(const EquivalenceConjecture& conj) {
        return std::unique_ptr<EquivalenceImplicationsImpl>(new EquivalenceImplicationsImpl(conj));
    }
    
    EquivalenceImplicationsImpl::~EquivalenceImplicationsImpl() {
        
    }
    
    const EquivalenceImplications::CommitResult test_commitWorkQueue(EquivalenceImplications& target) {
#ifndef CANDY_HAS_NO_RTTI
        EquivalenceImplicationsImpl* impl = dynamic_cast<EquivalenceImplicationsImpl*>(&target);
        assert(impl != nullptr);
#else
        EquivalenceImplicationsImpl* impl = reinterpret_cast<EquivalenceImplicationsImpl*>(&target);
#endif
        return impl->commitWorkQueue();
    }
    
    void test_commitWorkQueueWithoutDelta(EquivalenceImplications& target) {
#ifndef CANDY_HAS_NO_RTTI
        EquivalenceImplicationsImpl* impl = dynamic_cast<EquivalenceImplicationsImpl*>(&target);
        assert(impl != nullptr);
#else
        EquivalenceImplicationsImpl* impl = reinterpret_cast<EquivalenceImplicationsImpl*>(&target);
#endif
        impl->commitWorkQueueWithoutDelta();
    }
    
    /* -- ApproximationState implementation --------------------------------------------------------- */

    ApproximationState::ApproximationState() noexcept {
        
    }
    
    ApproximationState::~ApproximationState() {
        
    }
    
    class ApproximationStateImpl : public ApproximationState {
    public:
        explicit ApproximationStateImpl(const Conjectures& conjectures);
        
        equivalenceimplications_iterator beginEquivalenceImplications() noexcept override;
        equivalenceimplications_iterator endEquivalenceImplications() noexcept override;
        equivalenceimplications_size_type equivalenceImplicationsSize() const noexcept override;
        
        Backbones& getBackbones() const noexcept override;
        
        std::unique_ptr<ApproximationDelta> createInitializationDelta() override;
        std::unique_ptr<ApproximationDelta> createDelta() override;
        
        virtual ~ApproximationStateImpl();
        ApproximationStateImpl(const ApproximationStateImpl& other) = delete;
        ApproximationStateImpl& operator=(const ApproximationStateImpl& other) = delete;
        
    private:
        /** The collection of equivalences maintained by the ApproximationState. */
        std::deque<std::unique_ptr<EquivalenceImplicationsImpl>> m_equivalenceImplications;
        
        /** The collection of equivalences maintained by the ApproximationState (raw pointers). */
        std::deque<EquivalenceImplications*> m_equivalenceImplicationsRawPtr;
        
        /** The collection of backbone literals maintained by the ApproximationState. */
        std::unique_ptr<BackbonesImpl> m_backbones;
    };

    
    
    ApproximationState::equivalenceimplications_iterator ApproximationStateImpl::beginEquivalenceImplications() noexcept  {
        return m_equivalenceImplicationsRawPtr.begin();
    }
    
    ApproximationState::equivalenceimplications_iterator ApproximationStateImpl::endEquivalenceImplications() noexcept  {
        return m_equivalenceImplicationsRawPtr.end();
    }
    
    ApproximationState::equivalenceimplications_size_type
    ApproximationStateImpl::equivalenceImplicationsSize() const noexcept {
        return m_equivalenceImplicationsRawPtr.size();
    }
    
    Backbones& ApproximationStateImpl::getBackbones() const noexcept {
        return *m_backbones;
    }
    
    ApproximationStateImpl::ApproximationStateImpl(const Conjectures& conjectures)
    : ApproximationState(),
    m_equivalenceImplications(),
    m_equivalenceImplicationsRawPtr(),
    m_backbones(createBackbones(conjectures.getBackbones())) {
        for(auto& eq : conjectures.getEquivalences()) {
            m_equivalenceImplications.push_back(createEquivalenceImplications(eq));
            m_equivalenceImplicationsRawPtr.push_back(m_equivalenceImplications.back().get());
        }
    }
    
    std::unique_ptr<ApproximationDelta> ApproximationStateImpl::createDelta() {
        std::vector<EquivalenceImplications::CommitResult> eqCommitResults;
        
        for (auto&& equivalenceImpls : m_equivalenceImplications) {
            auto commitResult = equivalenceImpls->commitWorkQueue();
            eqCommitResults.push_back(commitResult);
        }
        
        auto bbCommitResult = m_backbones->commitWorkQueue();
        
        return createApproximationDelta(eqCommitResults, bbCommitResult);
    }
    
    std::unique_ptr<ApproximationDelta> ApproximationStateImpl::createInitializationDelta() {
        std::vector<EquivalenceImplications::CommitResult> eqCommitResults;
        EquivalenceImplications::CommitResult addedEquivalences;
        
        for (auto&& equivalenceImpls : m_equivalenceImplications) {
            equivalenceImpls->commitWorkQueueWithoutDelta();
            for (auto impl : *equivalenceImpls) {
                addedEquivalences.newImplications.push_back(impl);
            }
        }
        
        eqCommitResults.push_back(addedEquivalences);
        Backbones::CommitResult emptyBBCommitResult;
        m_backbones->commitWorkQueueWithoutDelta();
        
        return createApproximationDelta(eqCommitResults, emptyBBCommitResult);
    }
    
    ApproximationStateImpl::~ApproximationStateImpl() {
        
    }
    
    std::unique_ptr<ApproximationState> createApproximationState(const Conjectures& conjectures) {
        return std::unique_ptr<ApproximationState>(new ApproximationStateImpl(conjectures));
    }
}
