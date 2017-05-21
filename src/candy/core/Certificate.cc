/*
 * Certificate.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include "candy/core/Certificate.h"

namespace Candy {

Certificate::Certificate(const char* _out, const bool _active) :
                active(_out != nullptr && _active)
{
    if (active) {
        out = std::ofstream(_out, std::ofstream::out);
        if (!out.is_open()) {
            active = false;
        }
    }
    else {
        out = std::ofstream();
    }
}

Certificate::~Certificate() {
    if (active) {
        out.close();
    }
}

} /* namespace Candy */
