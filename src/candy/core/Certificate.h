/*
 * Certificate.h
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CERTIFICATE_H_
#define SRC_CANDY_CORE_CERTIFICATE_H_

#include <vector>
#include <cstring>
#include <fstream>
#include <memory>
#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/Clause.h"
#include "candy/utils/MemUtils.h"

namespace Candy {

class Clause;

class Certificate {
private:
    bool active;
    std::unique_ptr<std::ofstream> out;

    template<typename Iterator>
    void printLiterals(Iterator it, Iterator end) {
        for(; it != end; it++) {
            *out << (var(*it) + 1) * (sign(*it) ? -1 : 1) << " ";
        }
        *out << "0\n";
    }

    template<typename Iterator>
    void printLiteralsExcept(Iterator it, Iterator end, Lit lit) {
        for(; it != end; it++) {
            if (*it != lit) *out << (var(*it) + 1) * (sign(*it) ? -1 : 1) << " ";
        }
        *out << "0\n";
    }

public:
    Certificate(const char* _out) : active(false) {
        reset(_out);
    }

    ~Certificate() {
        reset(nullptr);
    }

    bool isActive() {
        return active;
    }

    void reset(const char* target) {
        if (target == nullptr) {
            if (out && out->is_open()) out->close();
            active = false;
        }
        else {
            std::unique_ptr<std::ofstream> new_out = backported_std::make_unique<std::ofstream>(target, std::ofstream::out);

            if (new_out->is_open()) {
                if (out && out->is_open()) out->close();
                this->out = std::move(new_out);
                this->active = true;
            }
        }
    }

    void proof() {
        if (active) {
            *out << "0" << std::endl;
        }
    }

    void added(std::initializer_list<Lit> list) {
        added(list.begin(), list.end());
    }

    void removed(std::initializer_list<Lit> list) {
        removed(list.begin(), list.end());
    }

    template<typename Iterator>
    void added(Iterator it, Iterator end) {
        if (active) {
            printLiterals(it, end);
        }
    }

    template<typename Iterator>
    void removed(Iterator it, Iterator end) {
        if (active) {
            *out << "d ";
            printLiterals(it, end);
        }
    }

    template<typename Iterator>
    void strengthened(Iterator it, Iterator end, Lit lit) {
        if (active) {
            printLiteralsExcept(it, end, lit);
            removed(it, end);
        }
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CERTIFICATE_H_ */
