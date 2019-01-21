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

CandySolverInterface* createSolver(bool staticPropagate, bool lrb, bool rsil, unsigned int rsil_adv_size) {
    CandyBuilder<> builder { }; 

    if (lrb) {
        if (staticPropagate) {
            return builder.branchWithLRB().propagateStaticClauses().build();
        } else {
            return builder.branchWithLRB().build();
        }
    } 
    else if (rsil) {
        RSILMode mode = getRSILMode();
        if (staticPropagate) {
            if (rsil_adv_size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.propagateStaticClauses().branchWithRSILUnrestricted3().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.propagateStaticClauses().branchWithRSILVanishing3().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.propagateStaticClauses().branchWithRSILBudgeted3().build();
                }
            }
            else if (rsil_adv_size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.propagateStaticClauses().branchWithRSILUnrestricted2().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.propagateStaticClauses().branchWithRSILVanishing2().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.propagateStaticClauses().branchWithRSILBudgeted2().build();
                }
            }
        } else {
            if (rsil_adv_size == 3) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.branchWithRSILUnrestricted3().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.branchWithRSILVanishing3().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.branchWithRSILBudgeted3().build();
                }
            }
            else if (rsil_adv_size == 2) {
                if (mode == RSILMode::UNRESTRICTED) {
                    return builder.branchWithRSILUnrestricted2().build();
                }
                else if (mode == RSILMode::VANISHING) {
                    return builder.branchWithRSILVanishing2().build();
                }
                else if (mode == RSILMode::IMPLICATIONBUDGETED) {
                    return builder.branchWithRSILBudgeted2().build();
                }
            }
        }
    }
    else {
        if (staticPropagate) {
            return builder.propagateStaticClauses().build();
        } else {
            return builder.build();
        }
    }
}

}