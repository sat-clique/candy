/*
 * Timeout.h
 *
 *  Created on: May 18, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_UTILS_RUNTIME_H_
#define SRC_CANDY_UTILS_RUNTIME_H_

#include <candy/utils/System.h>
#include <chrono>
#include <iostream>

namespace Candy {

class Runtime {
private:
    std::chrono::milliseconds startTime;
    std::chrono::milliseconds totalRuntime;
    std::chrono::milliseconds timeout;

public:
    Runtime(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) {
        this->timeout = timeout;
        this->startTime = std::chrono::milliseconds{0};
        this->totalRuntime = std::chrono::milliseconds{0};

    }
    ~Runtime() {}

    bool start() {
        if (startTime != std::chrono::milliseconds{0}) {
            return false;
        }
        startTime = Glucose::cpuTime();
        return true;
    }

    bool stop() {
        if (startTime == std::chrono::milliseconds{0}) {
            return false;
        }
        totalRuntime += Glucose::cpuTime() - this->startTime;
        return true;
    }

    bool hasTimeout() {
        return timeout > std::chrono::milliseconds{0} && getRuntime() > timeout;
    }

    void setTimeout(std::chrono::milliseconds timeout) {
        this->timeout = timeout;
    }

    std::chrono::milliseconds getRuntime() {
        if (this->startTime > std::chrono::milliseconds{0}) {
            return Glucose::cpuTime() - this->startTime;
        }
        return std::chrono::milliseconds{-1};
    }
};

}

#endif /* SRC_CANDY_UTILS_RUNTIME_H_ */
