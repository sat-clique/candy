#ifndef GBD_HASH_H
#define GBD_HASH_H

#include <string>

#include "candy/core/CNFProblem.h"

class Hash {
private:
    const Candy::CNFProblem& problem;

public:
    Hash(const Candy::CNFProblem& problem_);

    std::string gbd_hash();
    std::string degree_hash();

};

#endif