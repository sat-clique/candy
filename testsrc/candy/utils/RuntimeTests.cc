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
#include <candy/utils/Runtime.h>
#include <candy/utils/System.h>

#include <chrono>
#include <thread>

namespace Candy {
    namespace {
        class TestTimeProvider {
        public:
            void advanceTime(std::chrono::milliseconds amount) {
                m_currentTime += amount;
            }
            
            std::chrono::milliseconds getTime() const noexcept {
                return m_currentTime;
            }
            
        private:
            std::chrono::milliseconds m_currentTime{2000};
        };
    }
    
    TEST(RuntimeTests, detectsTimeout) {
        GenericRuntime<TestTimeProvider> underTest;
        underTest.setTimeout(std::chrono::milliseconds{300});
        underTest.start();
        EXPECT_FALSE(underTest.hasTimeout());
        
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{150});
        EXPECT_FALSE(underTest.hasTimeout());
        
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{200});
        EXPECT_TRUE(underTest.hasTimeout());
    }
    
    TEST(RuntimeTests, runtimeCanBeInterrupted) {
        GenericRuntime<TestTimeProvider> underTest;
        
        underTest.start();
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{100});
        underTest.stop();
        EXPECT_EQ(underTest.getRuntime().count(), std::chrono::milliseconds{100}.count());
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{100});

        underTest.start();
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{100});
        underTest.stop();
        EXPECT_EQ(underTest.getRuntime().count(), std::chrono::milliseconds{200}.count());
    }
    
    TEST(RuntimeTests, runtimeCanBeLapped) {
        GenericRuntime<TestTimeProvider> underTest;
        
        underTest.start();
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{100});
        auto lap1 = underTest.lap();
        
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{150});
        auto lap2 = underTest.lap();
        
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{200});
        auto lap3 = underTest.lap();
        
        EXPECT_EQ(lap1.count(), std::chrono::milliseconds{100}.count());
        EXPECT_EQ(lap2.count(), std::chrono::milliseconds{150}.count());
        EXPECT_EQ(lap3.count(), std::chrono::milliseconds{200}.count());
    }
    
    TEST(RuntimeTests, runtimeCanBeLappedWithInterrupt) {
        GenericRuntime<TestTimeProvider> underTest;
        
        underTest.start();
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{100});
        auto lap1 = underTest.lap();
        
        underTest.stop();
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{150});
        underTest.start();
        
        underTest.test_getTimeProvider().advanceTime(std::chrono::milliseconds{200});
        auto lap2 = underTest.lap();
        
        EXPECT_EQ(lap1.count(), std::chrono::milliseconds{100}.count());
        EXPECT_EQ(lap2.count(), std::chrono::milliseconds{200}.count());
    }
}
