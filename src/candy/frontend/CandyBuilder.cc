#include "candy/frontend/CandyBuilder.h"

namespace Candy {

enum class RSILMode {
    UNRESTRICTED, ///< use RSIL for all decisions
    VANISHING, ///< use RSIL with decreasing probability (note that the pseudorandom number generator is determinized)
    IMPLICATIONBUDGETED ///< use RSIL with implication budgets
};

RSILMode getRSILMode() {
    const std::string& mode = std::string{RSILOptions::opt_rsil_mode};
    if (mode == "unrestricted") {
        return RSILMode::UNRESTRICTED;
    }
    else if (mode == "vanishing") {
        return RSILMode::VANISHING;
    }
    else if (mode == "implicationbudgeted") {
        return RSILMode::IMPLICATIONBUDGETED;
    }
    else {
        throw std::invalid_argument("Error: unknown RSIL mode " + mode);
    }
}

CandySolverInterface* createSolver(GlobalClauseAllocator* global_allocator, bool staticPropagate, bool lrb, bool rsil) {
    ClauseDatabase clauses;
    Trail* assignment = new Trail();

    if (global_allocator != nullptr) {
        clauses.setGlobalClauseAllocator(global_allocator);
    }

    CandyBuilder<> builder { }; 

    if (lrb) {
        if (staticPropagate) {
            return builder.branchWithLRB().propagateStaticClauses().build(std::move(clauses), assignment);
        } else {
            return builder.branchWithLRB().build(std::move(clauses), assignment);
        }
    } 
    else if (rsil) {
        RSILMode mode = getRSILMode();
        unsigned int size = 3;
        if (staticPropagate) {
            if (size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.propagateStaticClauses().branchWithRSILUnrestricted3().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.propagateStaticClauses().branchWithRSILVanishing3().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.propagateStaticClauses().branchWithRSILBudgeted3().build(std::move(clauses), assignment);
                }
            }
            else if (size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.propagateStaticClauses().branchWithRSILUnrestricted2().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.propagateStaticClauses().branchWithRSILVanishing2().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.propagateStaticClauses().branchWithRSILBudgeted2().build(std::move(clauses), assignment);
                }
            }
        } else {
            if (size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.branchWithRSILUnrestricted3().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.branchWithRSILVanishing3().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.branchWithRSILBudgeted3().build(std::move(clauses), assignment);
                }
            }
            else if (size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.branchWithRSILUnrestricted2().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.branchWithRSILVanishing2().build(std::move(clauses), assignment);
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.branchWithRSILBudgeted2().build(std::move(clauses), assignment);
                }
            }
        }
    }
    else {
        if (staticPropagate) {
            return builder.propagateStaticClauses().build(std::move(clauses), assignment);
        } else {
            return builder.build(std::move(clauses), assignment);
        }
    }
}

}