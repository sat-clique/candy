/*
 * Certificate.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include "candy/core/Certificate.h"

namespace Candy {

Certificate::Certificate(const char* _out, const bool _active) :
                active(_out != nullptr && _active),
                out(_out, std::ofstream::out)
{
    if (active) {;
        if (!out.is_open()) {
            active = false;
        }
    }
}

Certificate::~Certificate() {
    if (active) {
        out.close();
    }
}

} /* namespace Candy */
