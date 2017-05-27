/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
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

#include <gtest/gtest.h>
#include <utils/Runtime.h>
#include <utils/System.h>

#include <chrono>
#include <thread>

namespace Candy {
    namespace {
        void busyWait(std::chrono::milliseconds duration) {
            auto startTime = Glucose::cpuTime();
            volatile uint64_t counter = 0;
            while (Glucose::cpuTime() - startTime < duration) {
                ++counter;
            }
        }
    }
    
    TEST(RuntimeTests, measuresCPUTime) {
        Runtime underTest;
        underTest.start();
        EXPECT_LE(underTest.getRuntime(), std::chrono::milliseconds{1});
        busyWait(std::chrono::milliseconds{10});
        EXPECT_NEAR(static_cast<double>(underTest.getRuntime().count()),
                    static_cast<double>(std::chrono::milliseconds{10}.count()),
                    1.0f /* absolute allowed error */);
    }
    
    TEST(RuntimeTests, doesNotMeasureWallClockTime) {
        Runtime underTest;
        underTest.start();
        EXPECT_LE(underTest.getRuntime(), std::chrono::milliseconds{1});
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        EXPECT_LE(underTest.getRuntime(), std::chrono::milliseconds{1});
    }
    
    TEST(RuntimeTests, detectsTimeout) {
        Runtime underTest;
        underTest.setTimeout(std::chrono::milliseconds{50});
        underTest.start();
        EXPECT_FALSE(underTest.hasTimeout());
        
        busyWait(std::chrono::milliseconds{10});
        EXPECT_FALSE(underTest.hasTimeout());
        
        busyWait(std::chrono::milliseconds{45});
        EXPECT_TRUE(underTest.hasTimeout());
    }
}
