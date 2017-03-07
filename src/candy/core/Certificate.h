/*
 * Certificate.h
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CERTIFICATE_H_
#define SRC_CANDY_CORE_CERTIFICATE_H_

#include <vector>
#include <cstdio>
#include <cstring>
#include <candy/core/SolverTypes.h>

namespace Candy {

class Clause;

class Certificate {
private:
    FILE* out;
    bool active;

public:
    Certificate(const char* _out, bool _active);
    virtual ~Certificate();

    void learnt(std::vector<Lit>& vec);
    void learntExcept(class Clause* c, Lit p);
    void removed(class Clause* c);
    void removed(std::vector<Lit>& vec);
    void proof();
    bool isActive();
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CERTIFICATE_H_ */
