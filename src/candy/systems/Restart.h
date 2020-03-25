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

#include "candy/mtl/BoundedQueue.h"

namespace Candy {

class Restart {
    ClauseDatabase& clause_db;
    Trail& trail;

    unsigned int restarts;

    double K;
    double R;
    float sumLBD = 0;
    bqueue<uint32_t> lbdQueue, trailQueue;

public:
    Restart(ClauseDatabase& clause_db_, Trail& trail_)
     : clause_db(clause_db_), trail(trail_), restarts(0), 
        K(SolverOptions::opt_K), 
        R(SolverOptions::opt_R), 
        sumLBD(0),
        lbdQueue(SolverOptions::opt_size_lbd_queue), 
        trailQueue(SolverOptions::opt_size_trail_queue) { }
    
    ~Restart() { }

    unsigned int nRestarts() const {
        return restarts;
    }

    inline void process_conflict() {
        trailQueue.push(trail.size());
        if (clause_db.result.nConflicts > 10000 && lbdQueue.isvalid() && trail.size() > R * trailQueue.getavg()) {
            lbdQueue.fastclear(); // BLOCK RESTART (CP 2012 paper)
        }
        lbdQueue.push(clause_db.result.lbd);
        sumLBD += clause_db.result.lbd;
    }

    inline bool trigger_restart() {
        if (clause_db.result.nConflicts > 0 && lbdQueue.isvalid() && ((lbdQueue.getavg() * K) > (sumLBD / clause_db.result.nConflicts))) {
            lbdQueue.fastclear();
            restarts++;
            return true;
        }
        return false;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
