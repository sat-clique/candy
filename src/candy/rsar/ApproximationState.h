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

#ifndef X_500114B7_8000_48A9_B794_0AB186344A6E_REFINEMENTSTATE_H
#define X_500114B7_8000_48A9_B794_0AB186344A6E_REFINEMENTSTATE_H

#include <core/SolverTypes.h>

#include <memory>
#include <deque>
#include <vector>

namespace Candy {
    class Conjectures;
    class EquivalenceConjecture;
    class BackboneConjecture;
    
    using Implication = std::pair<Lit, Lit>;
    
    /**
     * \class Backbones
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A data structure for committing and computing backbone removals.
     *
     * ApproximationState uses objects of this class to represent the currently active
     * backbones. Users of ApproximationState can deactivate backbones by adding them
     * to the Backbones object's variable removal queue and obtaining ApproximationState
     * an approximation delta from the ApproximationState object.
     */
    class Backbones {
    public:
        struct CommitResult {
            std::vector<Lit> removedBackbones;
        };
        
        /**
         * Requests the removal of backbone literals having the given variable. Note
         * that this change is committed lazily.
         */
        virtual void addVariableRemovalToWorkQueue(Var it) = 0;
        
        /**
         * Returns the set of represented backbone literals.
         */
        virtual const std::vector<Lit>& getBackboneLiterals() = 0;
        
        Backbones();
        virtual ~Backbones();
        Backbones(const Backbones& other) = delete;
        Backbones& operator=(const Backbones& other) = delete;
    };
    
    
    /**
     * \class EquivalenceImplications
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A data structure for committing and computing removals from literal equivalence
     * classes.
     *
     * ApproximationState uses objects of this class to represent the currently active
     * implications of equivalence classes. Users of ApproximationState can remove equivalences
     * by adding the corresponding variables to the EquivalenceImplications object's
     * variable removal queue and obtaining a ApproximationDelta from the ApproximationState object.
     */
    class EquivalenceImplications {
    public:
        struct CommitResult {
        public:
            std::vector<Implication> newImplications;
            std::vector<Implication> removedImplications;
        };
        
        /**
         * Requests the removal of literals (having the given variable) from the represented
         * set of equivalent literals. Note that this change is committed lazily.
         */
        virtual void addVariableRemovalToWorkQueue(Var it) = 0;
        
        /**
         * Returns a reference to the collection of implications encoding the represented
         * equivalences.
         */
        virtual const std::vector<Implication>& getImplications() const = 0;
        
        EquivalenceImplications();
        virtual ~EquivalenceImplications();
        EquivalenceImplications(const EquivalenceImplications& other) = delete;
        EquivalenceImplications& operator=(const EquivalenceImplications& other) = delete;
    };
    
    
    /**
     * \class ApproximationDelta
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A representation of changes in the set of equivalences and backbones.
     *
     */
    class ApproximationDelta {
    public:
        using const_backbone_iterator = std::vector<Lit>::const_iterator;
        using const_backbone_size_type = std::vector<Lit>::size_type;
        
        using const_implication_iterator = std::vector<Implication>::const_iterator;
        using const_implication_size_type = std::vector<Implication>::size_type;
        
        /**
         * Returns a begin iterator for the removed implications.
         */
        virtual const_implication_iterator beginRemovedImplications() const = 0;
        
        /**
         * Returns an end iterator for the removed implications.
         */
        virtual const_implication_iterator endRemovedImplications() const = 0;
        
        /**
         * Returns the amount of removed implications.
         */
        virtual const_implication_size_type removedImplicationsSize() const = 0;
        
        /**
         * Returns a begin iterator for the added implications.
         */
        virtual const_implication_iterator beginAddedImplications() const = 0;
        
        /**
         * Returns an end iterator for the added implications.
         */
        virtual const_implication_iterator endAddedImplications() const = 0;
        
        /**
         * Returns the amount of added implications.
         */
        virtual const_implication_size_type addedImplicationsSize() const = 0;
        
        /**
         * Returns a begin iterator for the removed backbone literals.
         */
        virtual const_backbone_iterator beginRemovedBackbones() const = 0;
        
        /**
         * Returns an end iterator for the removed backbone literals.
         */
        virtual const_backbone_iterator endRemovedBackbones() const = 0;
        
        /**
         * Returns the amount of removed backbone literals.
         */
        virtual const_backbone_size_type removedBackbonesSize() const = 0;
        
        ApproximationDelta();
        virtual ~ApproximationDelta();
        ApproximationDelta(const ApproximationDelta& other) = delete;
        ApproximationDelta& operator= (const ApproximationDelta& other) = delete;
    };
    
    
    
    /**
     * \class ApproximationState
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief ApproximationState objects maintain the set of active equivalences and backbone literals,
     *  creating deltas for changes in the set of equivalences rsp. backbone literals.
     *
     * ApproximationState maintains a set of equivalence classes represented by literal implications and a
     * collection of backbone literals. Users of ApproximationState may issue changes to the represented
     * implications rsp. the collection of backbone literals, which ApproximationState commits in the sense
     * of maintaining the equivalence classes' integrity (adding "patch" implications as needed).
     * The changes are made available to the user in the form of a ApproximationDelta.
     *
     * Intended usage: create a ApproximationState object using conjectures about literal equivalences and
     * backbone variables, e.g. obtained using random simulation, and iteratively refine the set of
     * equivalences rsp. backbone literals by committing changes to the EquivalenceImplications rsp.
     * Backbones objects. At each step, obtain a corresponding ApproximationDelta containing the
     * removed items as well as new implications needed to maintain the represented equivalences.
     */
    class ApproximationState {
    public:
        using equivalenceimplications_iterator = std::deque<EquivalenceImplications*>::iterator;
        using equivalenceimplications_size_type = std::deque<EquivalenceImplications*>::size_type;
        
        /**
         * Returns a begin iterator for the collection of equivalence-class representations.
         * Users may add variables to the removal queues of these objects.
         */
        virtual equivalenceimplications_iterator beginEquivalenceImplications() = 0;
        
        /**
         * Returns an end iterator for the collection of equivalence-class representations.
         */
        virtual equivalenceimplications_iterator endEquivalenceImplications() = 0;
        
        /**
         * Returns the amount of equivalence-class representations.
         * Users may add variables to the removal queue of this objects.
         */
        virtual equivalenceimplications_size_type equivalenceImplicationsSize() const = 0;
        
        /**
         * Returns the collection of backbone literals.
         */
        virtual Backbones& getBackbones() const = 0;
        
        /**
         * Commits the removals queued by the user in the EquivalenceImplications and Backbones
         * objects and creates a corresponding ApproximationDelta. New implications are omitted
         * from the resulting delta.
         */
        virtual std::unique_ptr<ApproximationDelta> createInitializationDelta() = 0;
        
        /**
         * Commits the removals queued by the user in the EquivalenceImplications and Backbones
         * objects and creates a corresponding ApproximationDelta. The amount of new implications
         * contained in the result at most equal to the corresponding amount of removed
         * implications.
         */
        virtual std::unique_ptr<ApproximationDelta> createDelta() = 0;
        
        ApproximationState();
        virtual ~ApproximationState();
        ApproximationState(const ApproximationState& other) = delete;
        ApproximationState& operator=(const ApproximationState& other) = delete;
    };
    
    
    /**
     * Creates a ApproximationState instance using the equivalence classes and backbone
     * variables represented by the given Conjectures object.
     */
    std::unique_ptr<ApproximationState> createApproximationState(const Conjectures& conjectures);
    
}


namespace std {
    
    template <>
    struct hash<Candy::Implication>
    {
        std::size_t operator()(const Candy::Implication& key) const
        {
            return std::hash<Candy::Lit>()(key.first) ^ std::hash<Candy::Lit>()(key.second);
        }
    };
    
}


// Testing interface
#ifdef CANDY_EXPOSE_TESTING_INTERFACE
namespace Candy {
    /**
     * Commits the changes in the removal work queue, returning the removed
     * implications as well as the generated "patch" implications in a
     * CommitResult.
     *
     * This function must only be called with objects created by
     *   createEquivalenceImplications(const EquivalenceConjecture& conjs);
     */
    const EquivalenceImplications::CommitResult test_commitWorkQueue(EquivalenceImplications& target);
    
    /**
     * Commits the changes in the removal work queue, generating "patch" implications
     * as needed.
     *
     * This function must only be called with objects created by
     *   createBackbones(const EquivalenceConjecture& conjs);
     */
    void test_commitWorkQueueWithoutDelta(EquivalenceImplications& target);
    
    /**
     * Commits the changes in the removal work queue, returning the removed
     * backbone literals in a CommitResult.
     *
     * This function must only be called with objects created by
     *   createBackbones(const EquivalenceConjecture& conjs);
     */
    const Backbones::CommitResult test_commitWorkQueue(Backbones& target);
    
    /**
     * Commits the changes in the removal work queue.
     *
     * This function must only be called with objects created by
     *   createBackbones(const EquivalenceConjecture& conjs);
     *
     */
    void test_commitWorkQueueWithoutDelta(Backbones& target);
    
    std::unique_ptr<Backbones> createBackbones(const std::vector<BackboneConjecture>& conjectures);

    std::unique_ptr<EquivalenceImplications> createEquivalenceImplications(const EquivalenceConjecture& conjs);

    std::unique_ptr<ApproximationDelta> createApproximationDelta(const std::vector<const EquivalenceImplications::CommitResult> eqCommitRes,
                                                           const Backbones::CommitResult bbComitRes);
}
#endif

#endif
