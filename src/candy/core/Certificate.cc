/*
 * Certificate.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include "candy/core/Certificate.h"
#include "candy/utils/MemUtils.h"

namespace Candy {

Certificate::Certificate(const char* _out) : active(false) {
    reset(_out);
}

Certificate::~Certificate() {
    reset(nullptr);
}

void Certificate::reset(const char* target) {
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

} /* namespace Candy */
