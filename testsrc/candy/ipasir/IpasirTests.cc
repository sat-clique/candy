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

extern "C" {
#include <candy/ipasir/ipasir.h>
}

#include <iostream>
#include <algorithm>

namespace Candy {

    TEST(IpasirTest, ipasir_test_empty_clause) {
        printf("%s", ipasir_signature());
        
        void* solver = ipasir_init();
        
		ipasir_add(solver, 0);
		
		int result = ipasir_solve(solver);
		
        ASSERT_EQ(20, result);
        
        ipasir_release(solver);
    }


    TEST(IpasirTest, ipasir_test_one_lit) {
        printf("%s", ipasir_signature());

        void* solver = ipasir_init();

        ipasir_add(solver, 1);
        ipasir_add(solver, 0);

        int result = ipasir_solve(solver);

        ASSERT_EQ(10, result);

        int value = ipasir_val(solver, 1);
        ASSERT_EQ(1, value);

        ipasir_release(solver);
    }


    TEST(IpasirTest, ipasir_test_assume) {
        printf("%s", ipasir_signature());

        void* solver = ipasir_init();

        ipasir_add(solver, 1);
        ipasir_add(solver, 0);

        ipasir_assume(solver, -1);
        int result = ipasir_solve(solver);
        ASSERT_EQ(20, result);

        result = ipasir_solve(solver);
        ASSERT_EQ(10, result);
        int value = ipasir_val(solver, 1);
        ASSERT_EQ(1, value);

        ipasir_release(solver);
    }


    TEST(IpasirTest, ipasir_test_failed) {
        printf("%s", ipasir_signature());

        void* solver = ipasir_init();

        ipasir_add(solver, 1);
        ipasir_add(solver, 0);
        ipasir_assume(solver, -1);
        int result = ipasir_solve(solver);
        ASSERT_EQ(20, result);
        ASSERT_EQ(1, ipasir_failed(solver, -1));

        result = ipasir_solve(solver);
        int value = ipasir_val(solver, 1);
        ASSERT_EQ(10, result);
        ASSERT_EQ(1, value);

        ipasir_add(solver, 2);
        ipasir_add(solver, 0);
        ipasir_assume(solver, -1);
        ipasir_assume(solver, 2);
        result = ipasir_solve(solver);
        ASSERT_EQ(20, result);
        ASSERT_EQ(1, ipasir_failed(solver, -1));
        ASSERT_EQ(0, ipasir_failed(solver, 2));

        result = ipasir_solve(solver);
        ASSERT_EQ(10, result);
        ASSERT_EQ(1, ipasir_val(solver, 1));
        ASSERT_EQ(2, ipasir_val(solver, 2));

        ipasir_release(solver);
    }
    
}
