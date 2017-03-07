/*
 * Certificate.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include <candy/core/Certificate.h>
#include <candy/core/Clause.h>
#include <candy/core/SolverTypes.h>

namespace Candy {

Certificate::Certificate(const char* _out, bool _active) : out(nullptr), active(_active) {
    if (active) {
        if (!strcmp(_out, "NULL")) {
            out = fopen("/dev/stdout", "wb");
        } else {
            out = fopen(_out, "wb");
        }
        if (out == nullptr) {
            active = false;
        } else {
            fprintf(out, "o proof DRUP\n");
        }
    }
}

Certificate::~Certificate() {
    if (active) {
        fclose(out);
    }
}

bool Certificate::isActive() {
    return active;
}

void Certificate::proof() {
    if (active) {
        fprintf(out, "0\n");
    }
}

void Certificate::learnt(std::vector<Lit>& vec) {
    if (active) {
        for (Lit lit : vec) {
            fprintf(out, "%i ", (var(lit) + 1) * (-2 * sign(lit) + 1));
        }
        fprintf(out, "0\n");
    }
}

void Certificate::learntExcept(Clause* c, Lit p) {
    if (active) {
        for (Lit lit : *c) {
            if (lit != p) fprintf(out, "%i ", (var(lit) + 1) * (-2 * sign(lit) + 1));
        }
        fprintf(out, "0\n");
    }
}

void Certificate::removed(Clause* c) {
    if (active) {
        fprintf(out, "d ");
        for (Lit lit : *c) {
            fprintf(out, "%i ", (var(lit) + 1) * (-2 * sign(lit) + 1));
        }
        fprintf(out, "0\n");
    }
}

void Certificate::removed(std::vector<Lit>& vec) {
    if (active) {
        fprintf(out, "d ");
        for (Lit lit : vec) {
            fprintf(out, "%i ", (var(lit) + 1) * (-2 * sign(lit) + 1));
        }
        fprintf(out, "0\n");
    }
}

} /* namespace Candy */
