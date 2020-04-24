/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


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

#ifndef SolverTypes_h
#define SolverTypes_h

#include <assert.h>
#include <stdint.h>

#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>

namespace Candy {

//=================================================================================================
// Variables, literals, lifted booleans:

// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

struct Var {
	int32_t id;

	inline Var() {
		this->id = -1;
	}

	inline Var(int32_t id) {
		this->id = id;
	}

	inline operator int() const {
		return id;
	}

	inline Var& operator++ () { // prefix ++
		++this->id;
		return *this;
	}

	inline Var operator++ (int) { // postfix ++
		Var result(*this);
		++this->id;
		return result;
	}

	inline bool operator <(Var v) const {
		return id < v.id;
	}

	inline bool operator <(int i) const {
		return id < i;
	}
};

#define var_Undef Var { -1 }

struct Lit {
	int32_t x;

	inline Lit() : Lit(-1) { }

	inline Lit(Var v) : Lit(v, false) { }

	inline Lit(Var v, bool sign) {
		this->x = v.id + v.id + (sign ? 1 : 0);
	}

	inline operator int() const {
		return x;
	}

	inline bool sign() const {
		return x & 1;
	}

	inline Var var() const {
		return Var(x >> 1);
	}

	inline Lit operator ~ () const {
		Lit q;
		q.x = x ^ 1;
		return q;
	}

	inline Lit operator ^ (bool b) const {
		Lit q;
		q.x = x ^ (b ? 1 : 0);
		return q;
	}

	inline bool operator == (Lit p) const {
		return x == p.x;
	}

	inline bool operator != (Lit p) const {
		return x != p.x;
	}

	inline bool operator < (Lit p) const {
		return x < p.x;
	} // '<' makes p, ~p adjacent in the ordering.
};

inline Var operator"" _V(unsigned long long n) {
    assert(n > 0);
    return static_cast<Var>(n-1);
}

inline Lit operator"" _L(unsigned long long n) {
    assert(n > 0);
    return Lit { Var { (int32_t)n-1 }, false };
}

const Lit lit_Undef(-1, false); // -2
const Lit lit_Error(-1, true); // -1


//=================================================================================================
// Lifted booleans:

#define l_True  (lbool((uint8_t)0))
#define l_False (lbool((uint8_t)1))
#define l_Undef (lbool((uint8_t)2))

class lbool {
	uint8_t value;

public:
	explicit lbool(uint8_t v) : value(v) {}
	explicit lbool(bool x) : value(!x) {}	

	bool operator ==(lbool b) const {
		return (((b.value & 2) & (value & 2))
				| (!(b.value & 2) & (value == b.value))) != 0;
	}
	bool operator !=(lbool b) const {
		return !(*this == b);
	}
	lbool operator ^(bool b) const {
		return value == 2 ? l_Undef : lbool((uint8_t)(value ^ (uint8_t)b));
	}

	lbool operator &&(lbool b) const {
		uint8_t sel = (this->value << 1) | (b.value << 3);
		uint8_t v = (0xF7F755F4 >> sel) & 3;
		return lbool(v);
	}

	lbool operator ||(lbool b) const {
		uint8_t sel = (this->value << 1) | (b.value << 3);
		uint8_t v = (0xFCFCF400 >> sel) & 3;
		return lbool(v);
	}
};

typedef std::vector<Lit> Cl;
typedef std::vector<Cl*> For;

inline std::ostream& operator <<(std::ostream& stream, Var const& var) {
	stream << var+1;
    return stream;
}

inline std::ostream& operator <<(std::ostream& stream, Lit const& lit) {
	if (lit.sign()) stream << "-";
	stream << lit.var();
    return stream;
}

inline std::ostream& operator <<(std::ostream& stream, lbool const& value) {
	stream << (value == l_True ? '1' : (value == l_False ? '0' : 'X'));
    return stream;
}

inline std::ostream& operator <<(std::ostream& stream, Cl const& clause) {
    for (Lit lit : clause) {
        stream << lit << " ";
    }
    return stream;
}

inline std::ostream& operator <<(std::ostream& stream, For const& formula) {
    for (const Cl* clause : formula) {
        stream << *clause << std::endl;
    }
    return stream;
}

inline void printLiteral(Lit lit) {
    std::cout << lit;
}

inline void printLiteral(Lit lit, std::vector<lbool> values) {
    lbool value = values[lit.var()] ^ lit.sign();
	std::cout << lit << ":" << value;
}

inline void printClause(Cl clause) {
    std::cout << clause << std::endl;
}

inline void printDIMACS(Cl clause) {
    std::cout << clause << "0" << std::endl;
}

inline void printFormula(For f) {
	std::cout << f << std::endl;
}

}

// add std::hash template specialization
namespace std {

template<>
struct hash<Candy::Lit> {
	std::size_t operator()(const Candy::Lit& key) const {
		Candy::Var hashedVar = key.var();
		if (key.sign() == 0) { 
			hashedVar = ~hashedVar;
		}
		return std::hash<uint32_t>()(hashedVar.id);
	}
};

template<>
struct hash<Candy::Var> {
	std::size_t operator()(const Candy::Var& key) const {
		return std::hash<uint32_t>()(key.id);
	}
};

}

#endif
