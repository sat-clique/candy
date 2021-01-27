/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


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
*************************************************************************************************/

#include "candy/utils/Options.h"

#include <algorithm>

namespace Candy {

void parseOptions(int& argc, char** argv, bool strict) {
    unsigned int i = 1, j = 1;
    for (; i < argc; i++){
        const char* str = argv[i];
        if (strstr(str, "--help") != nullptr) {
            printUsageAndExit(argc, argv, true);
        } 
        else {
            bool parsed_ok = false;
        
            for (unsigned int k = 0; !parsed_ok && k < Option::getOptionList().size(); k++){
                parsed_ok = Option::getOptionList()[k]->parse(argv[i]);
            }

            if (!parsed_ok) {
                if (strict && argv[i][0] == '-') {
                    fprintf(stderr, "ERROR! Unknown flag \"%s\".\n", argv[i]);
                    printUsageAndExit(argc, argv, true);
                }
                else {
                    argv[j++] = argv[i];
                }
            }
        }
    }
    argc -= (i - j);
}

void printUsageAndExit (int argc, char** argv, bool verbose) {
    fprintf(stderr, "c USAGE: %s [options] <input-file>\n", argv[0]);

    std::sort(Option::getOptionList().begin(), Option::getOptionList().end(), Option::OptionLt());

    const char* prev_cat  = NULL;
    const char* prev_type = NULL;

    for (unsigned int i = 0; i < Option::getOptionList().size(); i++){
        const char* cat  = Option::getOptionList()[i]->category;
        const char* type = Option::getOptionList()[i]->type_name;

        if (cat != prev_cat)
            fprintf(stderr, "\n%s OPTIONS:\n\n", cat);
        else if (type != prev_type)
            fprintf(stderr, "\n");

        Option::getOptionList()[i]->help(verbose);

        prev_cat  = Option::getOptionList()[i]->category;
        prev_type = Option::getOptionList()[i]->type_name;
    }

    fprintf(stderr, "\nHELP OPTIONS:\n\n");
    fprintf(stderr, "  --help        Print help message.\n");
    fprintf(stderr, "\n");
    exit(0);
}

}