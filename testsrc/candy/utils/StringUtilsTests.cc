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
#include <candy/utils/StringUtils.h>

#include <string>

namespace Candy {
    TEST(TokenizeByWhitespaceTest, emptyInput) {
        auto result = tokenizeByWhitespace<int>(std::string{""});
        EXPECT_TRUE(result.empty());
    }
    
    TEST(TokenizeByWhitespaceTest, onlyWhitespaceInput) {
        auto result = tokenizeByWhitespace<int>(std::string{"\t \n  "});
        EXPECT_TRUE(result.empty());
    }
    
    TEST(TokenizeByWhitespaceTest, singleIntInput) {
        auto result = tokenizeByWhitespace<int>(std::string{"\t 1\n  "});
        ASSERT_EQ(result.size(), 1ull);
        EXPECT_EQ(result[0], 1);
    }
    
    TEST(TokenizeByWhitespaceTest, singleFloatInput) {
        auto result = tokenizeByWhitespace<float>(std::string{"\t 1.5\n  "});
        ASSERT_EQ(result.size(), 1ull);
        EXPECT_EQ(result[0], 1.5f);
    }
    
    TEST(TokenizeByWhitespaceTest, multiIntInput) {
        auto result = tokenizeByWhitespace<int>(std::string{"\t 1\n 2 3 4 "});
        ASSERT_EQ(result.size(), 4ull);
        EXPECT_EQ(result, (std::vector<int>{1, 2, 3, 4}));
    }
    
    TEST(TokenizeByWhitespaceTest, multiFloatInput) {
        auto result = tokenizeByWhitespace<float>(std::string{"\t 1\n 2.1 3.2 4 "});
        ASSERT_EQ(result.size(), 4ull);
        EXPECT_EQ(result, (std::vector<float>{1.0f, 2.1f, 3.2f, 4.0f}));
    }
}
