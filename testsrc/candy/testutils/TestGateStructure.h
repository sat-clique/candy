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

#ifndef X_1EC0AC34_0ABE_4820_9A41_CD888D51FC30_TESTGATESTRUCTURE_H
#define X_1EC0AC34_0ABE_4820_9A41_CD888D51FC30_TESTGATESTRUCTURE_H

#include <memory>
#include <vector>
#include <core/SolverTypes.h>
#include <utils/CNFProblem.h>

namespace Candy {
    
    
    /**
     * \class GateStructureBuilder
     *
     * \ingroup TestUtils
     *
     * \brief A builder for gate structure encodings.
     */
    class GateStructureBuilder {
    public:
        /**
         * Adds an AND gate to the gate structure.
         */
        virtual GateStructureBuilder& withAnd(const std::vector<Lit>& inputs, Lit output) = 0;
        
        /**
         * Adds an OR gate to the gate structure.
         */
        virtual GateStructureBuilder& withOr(const std::vector<Lit>& inputs, Lit output) = 0;
        
        /**
         * Adds an XOR gate to the gate structure.
         */
        virtual GateStructureBuilder& withXor(const std::vector<Lit>& inputs, Lit output) = 0;
        
        /**
         * Builds the gate structure's encoding.
         */
        virtual std::unique_ptr<CNFProblem> build() = 0;
        
        GateStructureBuilder();
        virtual ~GateStructureBuilder();
        GateStructureBuilder(const GateStructureBuilder& other) = delete;
        GateStructureBuilder& operator=(const GateStructureBuilder& other) = delete;
    };
    
    /**
     * \ingroup TestUtils
     *
     * Creates a gate-structure builder without Plaisted-Greenbaum encoding.
     */
    std::unique_ptr<GateStructureBuilder> createGateStructureBuilder();
}

#endif
