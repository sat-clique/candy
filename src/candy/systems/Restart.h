/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef SRC_CANDY_CORE_RESTART_H_
#define SRC_CANDY_CORE_RESTART_H_

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"

namespace Candy {

//ExponentiallyMovingAverage
class EMA {
    double value;
    float alpha, beta;
    unsigned int wait, period;
public:
    EMA(double alpha_) : value(1), alpha(alpha_), beta(1), wait(1), period(1) {}
    void update(double next) { 
        value += beta * (next - value); 

        if (beta > alpha && --wait == 0) {
            wait = period = (2 * period);
            beta *= .5;
            if (beta < alpha) beta = alpha;
        }
    }
    operator double() const { return value; }
};

class Restart {
    ClauseDatabase& clause_db;
    Trail& trail;

    unsigned int restarts;

    uint64_t minimum_conflicts;

    EMA ema_lbd_narrow;
    EMA ema_lbd_wide;
    EMA ema_trail_narrow;
    EMA ema_trail_wide;

    double force;//1.25
    double block;//1.4

public:
    Restart(ClauseDatabase& clause_db_, Trail& trail_)
     : clause_db(clause_db_), trail(trail_), restarts(0), minimum_conflicts(1000),
        ema_lbd_narrow(3e-2), ema_lbd_wide(1e-5), ema_trail_narrow(1e-2), ema_trail_wide(1e-5),
        force(SolverOptions::opt_restart_force), block(SolverOptions::opt_restart_block) { }
    
    ~Restart() { }

    unsigned int nRestarts() const {
        return restarts;
    }

    inline void process_conflict() {
        ema_trail_narrow.update(trail.size());
        ema_trail_wide.update(trail.size());
        ema_lbd_narrow.update(clause_db.result.lbd);
        ema_lbd_wide.update(clause_db.result.lbd);
    }

    inline bool trigger_restart() {
        if (clause_db.result.nConflicts < minimum_conflicts) {
            return false;
        }
        else {
            // minimum_conflicts = clause_db.result.nConflicts + 2;
            minimum_conflicts += 2;
        }
        if (ema_lbd_narrow / ema_lbd_wide > force) {
            std::cout << "c Restart " << ema_lbd_narrow << " / " << ema_lbd_wide << " = " << ema_lbd_narrow / ema_lbd_wide << std::endl;
            if (ema_trail_narrow / ema_trail_wide > block) { // block restart
                std::cout << " c Blocked " << ema_trail_narrow << " / " << ema_trail_wide << " = " << ema_trail_narrow / ema_trail_wide << std::endl;
                return false;
            }
            restarts++;
            return true;
        } else 
            // std::cout << "No Restart " << ema_lbd_narrow << " / " << ema_lbd_wide << " = " << ema_lbd_narrow / ema_lbd_wide << std::endl;
        return false;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
