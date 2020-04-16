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

#ifndef CandyOptions
#define CandyOptions

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iostream>

#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#ifndef PRIu64
#define PRIu64 "lu"
#define PRIi64 "ld"
#endif

#include <vector>

namespace Candy {

//==================================================================================================
// Top-level option parse/help functions:

extern void parseOptions     (int& argc, char** argv, bool strict = false);
extern void printUsageAndExit(int  argc, char** argv, bool verbose = false);
extern void setUsageHelp     (const char* str);
extern void setHelpPrefixStr (const char* str);


//==================================================================================================
// Options is an abstract class that gives the interface for all types options:
class Option {
 protected:
    const char* name;
    const char* description;
    const char* category;
    const char* type_name;

    static std::vector<Option*>& getOptionList () { static std::vector<Option*> options; return options; }
    static const char*&  getUsageString() { static const char* usage_str; return usage_str; }
    static const char*&  getHelpPrefixString() { static const char* help_prefix_str = ""; return help_prefix_str; }

    struct OptionLt {
        bool operator()(const Option* x, const Option* y) {
            int test1 = strcmp(x->category, y->category);
            return test1 < 0 || test1 == 0 && strcmp(x->type_name, y->type_name) < 0;
        }
    };

    Option(const char* name_, const char* desc_, const char* cate_, const char* type_, const bool disabled_ = false) : 
      name(name_) , description(desc_), category(cate_), type_name(type_) { 
        if (!disabled_) {
            getOptionList().push_back(this);
        }
        else {
            std::cout << "c Option '" << name_ << "' has been disabled. You are not able to change the default setting." << std::endl;
        }
    }

 public:
    virtual ~Option() {}

    virtual bool parse             (const char* str)      = 0;
    virtual void help              (bool verbose = false) = 0;

    friend  void parseOptions      (int& argc, char** argv, bool strict);
    friend  void printUsageAndExit (int  argc, char** argv, bool verbose);
    friend  void setUsageHelp      (const char* str);
    friend  void setHelpPrefixStr  (const char* str);
};


//==================================================================================================
// Range classes with specialization for floating types:
struct IntRange {
    int begin;
    int end;
    IntRange(int b, int e) : begin(b), end(e) {}
};

struct Int64Range {
    int64_t begin;
    int64_t end;
    Int64Range(int64_t b, int64_t e) : begin(b), end(e) {}
};

struct DoubleRange {
    double begin;
    double end;
    bool  begin_inclusive;
    bool  end_inclusive;
    DoubleRange(double b, bool binc, double e, bool einc) : begin(b), end(e), begin_inclusive(binc), end_inclusive(einc) {}
};


//==================================================================================================
// Double options:
class DoubleOption : public Option {
    char pattern[100]; 
    unsigned int len;

 protected:
    DoubleRange range;
    double      value;

 public:
    DoubleOption(const char* c, const char* n, const char* d, double def = double(), DoubleRange r = DoubleRange(-HUGE_VAL, false, HUGE_VAL, false), const bool disabled_ = false)
        : Option(n, d, c, "<double>", disabled_), range(r), value(def) {
        // FIXME: set LC_NUMERIC to "C" to make sure that strtof/strtod parses decimal point correctly.
        len = sprintf(pattern, "-%s=", name);
    }

    operator      double   (void) const { return value; }
    DoubleOption& operator=(double x)   { value = x; return *this; }

    virtual bool parse(const char* str) {
        const char* span = strstr(str, pattern); 

        if (span == str) {
            char* end;
            double tmp = strtod(span + len, &end);

            if (end != nullptr) {
                if (tmp > range.end && tmp < range.begin) {
                    fprintf(stderr, "ERROR! value <%s> is out of range for option \"%s\".\n", span, name);
                    exit(1);
                }

                value = tmp;
                return true;
            }
        }

        return false;
    }

    void set(double val) {
        value = val;
    }

    virtual void help (bool verbose = false) {
        fprintf(stderr, "  -%-12s = %-8s %c%4.2g .. %4.2g%c (default: %g)\n", 
                name, type_name, 
                range.begin_inclusive ? '[' : '(', 
                range.begin,
                range.end,
                range.end_inclusive ? ']' : ')', 
                value);
        if (verbose){
            fprintf(stderr, "    %s\n", description);
        }
    }
};


//==================================================================================================
// Int options:
class IntOption : public Option {
    char pattern[100]; 
    unsigned int len;

 protected:
    IntRange range;
    int32_t  value;

 public:
    IntOption(const char* c, const char* n, const char* d, int32_t def = int32_t(), IntRange r = IntRange(INT32_MIN, INT32_MAX), const bool disabled_ = false)
        : Option(n, d, c, "<int32>", disabled_), range(r), value(def) {
            len = sprintf(pattern, "-%s=", name);
        }
 
    operator   int32_t   (void) const { return value; }
    IntOption& operator= (int32_t x)  { value = x; return *this; }

    virtual bool parse(const char* str) {
        const char* span = strstr(str, pattern);

        if (span == str) {
            char* end;
            int32_t tmp = strtol(span + len, &end, 10);

            if (end != nullptr) {
                if (tmp > range.end && tmp < range.begin) {
                    fprintf(stderr, "ERROR! value <%s> is out of range for option \"%s\".\n", span, name);
                    exit(1);
                }

                value = tmp;
                return true;
            }
        }

        return false;
    }

    void set(int32_t val) {
        value = val;
    }

    int32_t get() {
        return value;
    }

    virtual void help(bool verbose = false) {
        fprintf(stderr, "  -%-12s = %-8s [", name, type_name);
        if (range.begin == INT32_MIN)
            fprintf(stderr, "imin");
        else
            fprintf(stderr, "%4d", range.begin);

        fprintf(stderr, " .. ");
        if (range.end == INT32_MAX)
            fprintf(stderr, "imax");
        else
            fprintf(stderr, "%4d", range.end);

        fprintf(stderr, "] (default: %d)\n", value);
        if (verbose) {
            fprintf(stderr, "    %s\n", description);
        }
    }
};


// Leave this out for visual C++ until Microsoft implements C99 and gets support for strtoll.
#ifndef _MSC_VER

class Int64Option : public Option {
    char pattern[100]; 
    unsigned int len;

 protected:
    Int64Range range;
    int64_t  value;

 public:
    Int64Option(const char* c, const char* n, const char* d, int64_t def = int64_t(), Int64Range r = Int64Range(INT64_MIN, INT64_MAX), const bool disabled_ = false)
        : Option(n, d, c, "<int64>", disabled_), range(r), value(def) {
            len = sprintf(pattern, "-%s=", name);
        }
 
    operator     int64_t   (void) const { return value; }
    Int64Option& operator= (int64_t x)  { value = x; return *this; }

    virtual bool parse(const char* str) {
        const char* span = strstr(str, pattern); 

        if (span == str) {
            char* end;
            int64_t tmp = strtoll(span + len, &end, 10);

            if (end != nullptr) {
                if (tmp > range.end && tmp < range.begin) {
                    fprintf(stderr, "ERROR! value <%s> is out of range for option \"%s\".\n", span, name);
                    exit(1);
                }

                value = tmp;
                return true;
            }
        }

        return false;
    }

    void set(int64_t val) {
        value = val;
    }

    virtual void help(bool verbose = false) {
        fprintf(stderr, "  -%-12s = %-8s [", name, type_name);
        if (range.begin == INT64_MIN)
            fprintf(stderr, "imin");
        else
            fprintf(stderr, "%4" PRIi64, range.begin);

        fprintf(stderr, " .. ");
        if (range.end == INT64_MAX)
            fprintf(stderr, "imax");
        else
            fprintf(stderr, "%4" PRIi64, range.end);

        fprintf(stderr, "] (default: %" PRIi64")\n", value);
        if (verbose) {
            fprintf(stderr, "    %s\n", description);
        }
    }
};
#endif

//==================================================================================================
// String option:
class StringOption : public Option {
    char pattern[100]; 
    unsigned int len;
    char* value;

 public:
    StringOption(const char* c, const char* n, const char* d, const char* def = "", const bool disabled_ = false) 
        : Option(n, d, c, "<string>", disabled_) {
            len = sprintf(pattern, "-%s=", name);
            value = new char[256];
            set(def);
        }

    operator      const char*  (void) const     { return value; }
    StringOption& operator=    (const char* x)  { set(x); return *this; }

    virtual bool parse(const char* str) {
        const char* span = strstr(str, pattern); 

        if (span == str) {
            set(span + len);
            return true;
        }

        return false;
    }

    void set(const char* val) {
        // memset(value, 0, 256);
        strncpy(value, val, 256);
    }

    char* get() {
        return value;
    }

    virtual void help(bool verbose = false) {
        fprintf(stderr, "  -%-10s = %8s\n", name, type_name);
        if (verbose) {
            fprintf(stderr, "    %s\n", description);
        }
    }    
};


//==================================================================================================
// Bool option:
class BoolOption : public Option {
    char yes_pattern[100]; 
    char no_pattern[100]; 
    unsigned int yes_len;
    unsigned int no_len;
    bool value;

 public:
    BoolOption(const char* c, const char* n, const char* d, bool v, const bool disabled_ = false) 
        : Option(n, d, c, "<bool>", disabled_), value(v) {
            yes_len = sprintf(yes_pattern, "-%s", name);
            no_len = sprintf(no_pattern, "-no-%s", name);
        }

    operator    bool     (void) const { return value; }
    operator    bool&    (void)       { return value; }
    BoolOption& operator=(bool b)     { value = b; return *this; }

    virtual bool parse(const char* str) {
        const char* span = strstr(str, yes_pattern); 

        if (span == str && strlen(str) == yes_len) {
            value = true;
            return true;
        }

        span = strstr(str, no_pattern);
        if (span == str && strlen(str) == no_len) {
            value = false;
            return true;
        }

        return false;
    }

    void set(bool val) {
        value = val;
    }

    virtual void help(bool verbose = false) {
        fprintf(stderr, "  -%s, -no-%s (default: %s)\n", name, name, value ? "on" : "off");
        if (verbose) {
            fprintf(stderr, "    %s\n", description);
        }
    }
};


}

#endif
