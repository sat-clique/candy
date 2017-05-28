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

namespace Candy {

class Runtime {
private:
    std::chrono::milliseconds startTime;
    std::chrono::milliseconds totalRuntime;
    std::chrono::milliseconds timeout;
    std::chrono::milliseconds lastLap;

public:
    explicit Runtime(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) {
        this->timeout = timeout;
        this->startTime = std::chrono::milliseconds{0};
        this->totalRuntime = std::chrono::milliseconds{0};
        this->lastLap = std::chrono::milliseconds{0};
    }
    ~Runtime() {}

    bool start() noexcept {
        if (startTime != std::chrono::milliseconds{0}) {
            return false;
        }
        startTime = Glucose::cpuTime();
        lastLap = totalRuntime;
        return true;
    }

    bool stop() noexcept {
        if (startTime == std::chrono::milliseconds{0}) {
            return false;
        }
        totalRuntime += Glucose::cpuTime() - this->startTime;
        startTime = std::chrono::milliseconds{0};
        return true;
    }

    bool hasTimeout() const noexcept {
        return timeout > std::chrono::milliseconds{0} && getRuntime() > timeout;
    }

    void setTimeout(std::chrono::milliseconds timeout) {
        this->timeout = timeout;
    }

    std::chrono::milliseconds getRuntime() const noexcept {
        if (this->startTime > std::chrono::milliseconds{0}) {
            return totalRuntime + (Glucose::cpuTime() - this->startTime);
        }
        else {
            return totalRuntime;
        }
    }

    std::chrono::milliseconds lap() noexcept {
        auto currentTime = getRuntime();
        auto result = currentTime - this->lastLap;
        this->lastLap = currentTime;
        return result;
    }
};

}

#endif /* SRC_CANDY_UTILS_RUNTIME_H_ */
