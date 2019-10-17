/* Copyright (c) 2017 Markus Iser (github.com/udopia)

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

#include <candy/core/SolverTypes.h>
#include <candy/minimizer/Minimizer.h>

#include <iostream>
#include <algorithm>
#include <vector>

namespace Candy {

    typedef std::initializer_list<std::initializer_list<Lit>> formula;

    TEST(MinimizerTest, simpleMinimize) {
        CNFProblem simple_or { {{1_L}, {~1_L, 2_L, 3_L}, {1_L, ~2_L}, {1_L, ~3_L}} };
        CandySolverResult model { 1_L, 2_L, 3_L };
        Minimizer minimi(simple_or, model);
        minimi.mimimizeModel(false, false);

        Cl minimized = model.getMinimizedModelLiterals();

        ASSERT_EQ(minimized.size(), 2);

        bool firstLit = std::find(minimized.begin(), minimized.end(), 1_L) != minimized.end();
        bool secondLit = std::find(minimized.begin(), minimized.end(), 2_L) != minimized.end();
        bool thirdLit = std::find(minimized.begin(), minimized.end(), 3_L) != minimized.end();
        ASSERT_TRUE(firstLit);
        ASSERT_TRUE(secondLit || thirdLit);

    }
}
