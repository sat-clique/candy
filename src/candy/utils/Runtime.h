/*
 * Timeout.h
 *
 *  Created on: May 18, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_UTILS_RUNTIME_H_
#define SRC_CANDY_UTILS_RUNTIME_H_

#include <utils/System.h>

class Runtime {
private:
    double startTime;
    double totalRuntime;
    unsigned int timeout;

public:
    Runtime(unsigned int timeout = 0) {
        this->timeout = timeout;
        this->startTime = 0;
        this->totalRuntime = 0;

    }
    ~Runtime() {}

    bool start() {
        if (startTime != 0.0) {
            return false;
        }
        startTime = Glucose::cpuTime();
        return true;
    }

    bool stop() {
        if (startTime == 0.0) {
            return false;
        }
        totalRuntime += Glucose::cpuTime() - this->startTime;
        return true;
    }

    bool hasTimeout() {
        return timeout > 0 && getRuntime() > timeout;
    }

    bool setTimeout(unsigned int timeout) {
        this->timeout = timeout;
    }

    double getRuntime() {
        return totalRuntime + this->startTime > 0 ? Glucose::cpuTime() - this->startTime : 0.0;
    }
};

#endif /* SRC_CANDY_UTILS_RUNTIME_H_ */
