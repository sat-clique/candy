/***************************************************************************************[Main.cc]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 LRI  - Univ. Paris Sud, France (2009-2013)
 Labri - Univ. Bordeaux, France
 
 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 Labri - Univ. Bordeaux, France
 
 Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
 Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it
 is based on. (see below).
 
 Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
 version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
 without restriction, including the rights to use, copy, modify, merge, publish, distribute,
 sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 - The above and below copyrights notices and this permission notice shall be included in all
 copies or substantial portions of the Software;
 - The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
 be used in any competitive event (sat competitions/evaluations) without the express permission of
 the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
 using Glucose Parallel as an embedded SAT engine (single core or not).
 
 
 --------------- Original Minisat Copyrights
 
 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/


#ifndef X_FB1172A6_DFBF_4144_A597_41A66F8BC965_H
#define X_FB1172A6_DFBF_4144_A597_41A66F8BC965_H

#include <iostream>
#include <candy/frontend/GateAnalyzerFrontend.h>
#include <candy/frontend/RandomSimulationFrontend.h>
#include <candy/frontend/RSILSolverBuilder.h>
#include <candy/frontend/RSARFrontend.h>

namespace Candy {
    /**
     * \defgroup CandyFrontend
     */
    
    struct GlucoseArguments {
        const int verb;
        const bool mod;
        const int vv;
        
        const int cpu_lim;
        const int mem_lim;
        
        const bool do_solve;
        const bool do_preprocess;
        const bool do_certified;
        const bool do_gaterecognition;
        const bool do_simp_out;

        const int do_minimize;
        
        const char *opt_certified_file;
        
        const bool wait_for_user;
        
        const bool read_from_stdin;
        const char *input_filename;
        const char *output_filename;
        
        const GateRecognitionArguments gateRecognitionArgs;
        const RandomSimulationArguments randomSimulationArgs;
        const RSARArguments rsarArgs;
        const RSILArguments rsilArgs;
    };
    
    std::ostream& operator <<(std::ostream& stream, const GlucoseArguments& arguments);
    
    /**
     * \ingroup CandyFrontend
     *
     * \brief Parses the command line args for the Candy Kingdom. Exits the program when
     *   parsing the arguments failed.
     *
     * \param argv  the array of argument strings.
     * \param argc  the length of argv.
     *
     * \returns     the parsed arguments represented as a GlucoseArguments object.
     */
    GlucoseArguments parseCommandLineArgs(int& argc, char** argv);
}

#endif
