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

#include "CandyCommandLineParser.h"
#include "candy/frontend/CLIOptions.h" 

namespace Candy {
    std::ostream& operator <<(std::ostream& stream, const GlucoseArguments& arguments) {
        stream << "c Glucose arguments: " << std::endl
        << "c   Verbosity: " << arguments.verb << std::endl
        << "c   Show model: " << arguments.mod << std::endl
        << "c   CPU time limit: " << arguments.cpu_lim << std::endl
        << "c   Memory limit: " << arguments.mem_lim << std::endl
        << "c   Solve: " << arguments.do_solve << std::endl
        << "c   Preprocess: " << arguments.do_preprocess << std::endl
        << "c   Certified UNSAT: " << arguments.do_certified << std::endl
        << "c   Output Simplified Dimacs: " << arguments.do_simp_out << std::endl
        << "c   Benchmark gate recognition: " << arguments.do_gaterecognition << std::endl
        << "c   Certified UNSAT output: " << (arguments.opt_certified_file == nullptr ?
                                              "(none)" : arguments.opt_certified_file) << std::endl
        << "c   Wait for user: " << arguments.wait_for_user << std::endl
        << "c   Read problem from stdin: " << arguments.read_from_stdin << std::endl
        << "c   Input filename: " << (arguments.input_filename == nullptr ?
                                      "(none)" : arguments.input_filename) << std::endl;
        
        stream << arguments.gateRecognitionArgs;
        stream << arguments.randomSimulationArgs;
        stream << arguments.rsarArgs;
        stream << arguments.rsilArgs;
        
        return stream;
    }

    GlucoseArguments parseCommandLineArgs(int& argc, char** argv) {
        setUsageHelp("c USAGE: %s [options] <input-file>\n\nc where input may be either in plain or gzipped DIMACS.\n");
        
        parseOptions(argc, argv, true);
        
        GateRecognitionArguments gateRecognitionArgs{
            GateRecognitionOptions::opt_gr_tries,
            GateRecognitionOptions::opt_gr_patterns,
            GateRecognitionOptions::opt_gr_semantic,
            static_cast<unsigned int>(GateRecognitionOptions::opt_gr_semantic_budget),
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds{GateRecognitionOptions::opt_gr_timeout}),
            GateRecognitionOptions::opt_gr_holistic,
            GateRecognitionOptions::opt_gr_lookahead,
            GateRecognitionOptions::opt_gr_intensify,
            GateRecognitionOptions::opt_gr_lookahead_threshold,
            GateRecognitionOptions::opt_print_gates
        };
        
        RandomSimulationArguments rsArgs{
            RandomSimulationOptions::opt_rs_nrounds,
            RandomSimulationOptions::opt_rs_abortbyrrat,
            RandomSimulationOptions::opt_rs_rrat,
            RandomSimulationOptions::opt_rs_filterConjBySize > 0,
            RandomSimulationOptions::opt_rs_filterConjBySize,
            RandomSimulationOptions::opt_rs_removeBackboneConj,
            RandomSimulationOptions::opt_rs_filterGatesByNonmono,
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds{RandomSimulationOptions::opt_rs_ppTimeLimit})
        };
        
        RSARArguments rsarArgs{
            RSAROptions::opt_rsar_enable,
            RSAROptions::opt_rsar_maxRefinementSteps,
            parseSimplificationHandlingMode(std::string{RSAROptions::opt_rsar_simpMode}),
            std::string{RSAROptions::opt_rsar_inputDepCountHeurConf} != "",
            std::string{RSAROptions::opt_rsar_inputDepCountHeurConf},
            RSAROptions::opt_rsar_minGateCount
        };
        
        RSILArguments rsilArgs{
            RSILOptions::opt_rsil_enable,
            getRSILMode(std::string{RSILOptions::opt_rsil_mode}),
            static_cast<uint64_t>(RSILOptions::opt_rsil_vanHalfLife),
            static_cast<uint64_t>(RSILOptions::opt_rsil_impBudgets),
            RSILOptions::opt_rsil_filterByInputDeps != 0,
            RSILOptions::opt_rsil_filterByInputDeps,
            RSILOptions::opt_rsil_filterOnlyBackbone,
            RSILOptions::opt_rsil_minGateFraction,
            RSILOptions::opt_rsil_onlyMiters
        };

        bool readFromStdIn = (argc == 1);
        const char* inputFilename = (!readFromStdIn ? argv[1] : nullptr);
        
        return GlucoseArguments{
            SolverOptions::verb,
            SolverOptions::mod,
            SolverOptions::cpu_lim,
            SolverOptions::mem_lim,
            SolverOptions::do_solve,
            SolverOptions::do_preprocess,
            SolverOptions::do_certified,
            SolverOptions::do_gaterecognition,
            SolverOptions::do_simp_out,
            SolverOptions::do_minimize,
            SolverOptions::opt_certified_file,
            SolverOptions::wait_for_user,
            readFromStdIn,
            inputFilename,
            gateRecognitionArgs,
            rsArgs,
            rsarArgs,
            rsilArgs
        };
    }
}
