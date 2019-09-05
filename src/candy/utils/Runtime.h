/* Copyright (c) 2017 - 2019 Markus Iser
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.
 
 */

#ifndef SRC_CANDY_UTILS_RUNTIME_H_
#define SRC_CANDY_UTILS_RUNTIME_H_

#include <iomanip>

namespace Candy { 

double get_wall_time();
double get_cpu_time();

class Runtime {
private:
    double timeout;
    double startTime;

public:
    double totalRuntime;

    /**
     * Constructs a GenericRuntime object.
     *
     * \param timeout   the amount of time used for timeout detection. If 0, timeout
     *                  detection is disabled.
     */
    Runtime(double timeout_ = 0) : timeout(timeout_), startTime(0), totalRuntime(0) { }
    ~Runtime() {}

    /**
     * Starts time measurement.
     * 
     * This method resets the start time of the current lap.
     */
    bool start() noexcept {
        if (startTime != 0) {
            return false;
        }
        startTime = get_wall_time();
        return true;
    }

    /**
     * Stops time measurement. This method should only be called
     * if start() has been called and stop() has not been already
     * called since the last invocation of start(). Otherwise, this
     * method has no effect. The time elapsed since the last start()
     * invocation is counted added the total amount of elapsed time.
     * (Time elapsing after the invocation of stop() and before the
     * next invocation of start() is not counted toward the total
     * elapsed time.)
     *
     * \returns true    iff the last invocation of stop() (if any)
     *                  preceded the last invocation of start().
     */
    bool stop() noexcept {
        if (startTime == 0) {
            return false;
        }
        totalRuntime += get_wall_time() - this->startTime;
        startTime = 0;
        return true;
    }

    /**
     * \returns true    iff timeout measurement is enabeld and the
     *                  total amount of elapsed time exceeds the timeout.
     */
    bool hasTimeout() const noexcept {
        return timeout > 0 && getRuntime() > timeout;
    }

    /**
     * Sets the timeout value.
     *
     * \param timeout   the timeout value in milliseconds.
     */
    void setTimeout(double timeout) {
        this->timeout = timeout;
    }

    /**
     * \returns     the total elapsed time in milliseconds.
     */
    double getRuntime() const noexcept {
        if (this->startTime > 0) {
            return totalRuntime + (get_wall_time() - this->startTime);
        }
        else {
            return totalRuntime;
        }
    }

};

inline std::ostream& operator <<(std::ostream& stream, Runtime const& runtime) {
    stream << std::fixed << std::setw(11) << std::setprecision(6) << runtime.totalRuntime << " seconds" << std::endl;
    return stream;
}

class OutOfTimeException {};

}

#endif /* SRC_CANDY_UTILS_RUNTIME_H_ */
