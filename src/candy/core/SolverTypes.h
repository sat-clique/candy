/***************************************************************************************[SolverTypes.h]
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

#ifndef SolverTypes_h
#define SolverTypes_h

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include <vector>
#include <algorithm>
#include <functional>

namespace Candy {

//=================================================================================================
// Variables, literals, lifted booleans:

// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

typedef int Var;
#define var_Undef (-1)

struct Lit {
	int32_t x;

	operator int() const {
		return x;
	}

	// Use this as a constructor:
	friend Lit mkLit(Var var, bool sign);

	bool operator ==(Lit p) const {
		return x == p.x;
	}
	bool operator !=(Lit p) const {
		return x != p.x;
	}
	bool operator <(Lit p) const {
		return x < p.x;
	} // '<' makes p, ~p adjacent in the ordering.
};

inline Lit mkLit(Var var, bool sign = false) {
	Lit p;
	p.x = var + var + (int) sign;
	return p;
}
inline Lit operator ~(Lit p) {
	Lit q;
	q.x = p.x ^ 1;
	return q;
}
inline Lit operator ^(Lit p, bool b) {
	Lit q;
	q.x = p.x ^ (unsigned int) b;
	return q;
}
inline bool sign(Lit p) {
	return p.x & 1;
}
inline int var(Lit p) {
	return p.x >> 1;
}

// Mapping Literals to and from compact integers suitable for array indexing:
inline int toInt(Var v) {
	return v;
}
inline int toInt(Lit p) {
	return p.x;
}
inline Lit toLit(int i) {
	Lit p;
	p.x = i;
	return p;
}

const Lit lit_Undef = { -2 };  // }- Useful special constants.
const Lit lit_Error = { -1 };  // }

//=================================================================================================
// Lifted booleans:
//
// NOTE: this implementation is optimized for the case when comparisons between values are mostly
//       between one variable and one constant. Some care had to be taken to make sure that gcc 
//       does enough constant propagation to produce sensible code, and this appears to be somewhat
//       fragile unfortunately.

#define l_True  (lbool((uint8_t)0)) // gcc does not do constant propagation if these are real constants.
#define l_False (lbool((uint8_t)1))
#define l_Undef (lbool((uint8_t)2))

class lbool {
	uint8_t value;

public:
	explicit lbool(uint8_t v) :
			value(v) {
	}

	lbool() :
			value(0) {
	}
	explicit lbool(bool x) :
			value(!x) {
	}

	bool operator ==(lbool b) const {
		return ((b.value & 2) & (value & 2))
				| (!(b.value & 2) & (value == b.value));
	}
	bool operator !=(lbool b) const {
		return !(*this == b);
	}
	lbool operator ^(bool b) const {
		return lbool((uint8_t) (value ^ (uint8_t) b));
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

	friend int toInt(lbool l);
	friend lbool toLbool(int v);
};
inline int toInt(lbool l) {
	return l.value;
}
inline lbool toLbool(int v) {
	return lbool((uint8_t) v);
}


// Output Helper Functions
inline void printLiteral(Lit lit) {
    printf("%s%i", sign(lit)?"-":"", var(lit)+1);
}

inline void printLiteral(Lit lit, std::vector<lbool> values) {
    lbool value = values[var(lit)] ^ sign(lit);
    printf("%s%d:%c", sign(lit) ? "-" : "", var(lit) + 1, value == l_True ? '1' : (value == l_False ? '0' : 'X'));
}

//=================================================================================================
// OccLists -- a class for maintaining occurence lists with lazy deletion:

template<class Idx, class Elem, class Deleted>
class OccLists {
	std::vector<std::vector<Elem>> occs;
	std::vector<char> dirty;
	std::vector<Idx> dirties;
	Deleted deleted;

public:
	OccLists() { }
	OccLists(const Deleted& d) : deleted(d) { }

	void init(const Idx& idx) {
		if ((int) occs.size() < toInt(idx) + 1)
			occs.resize(toInt(idx) + 1);
		if ((int) dirty.size() < toInt(idx) + 1)
			dirty.resize(toInt(idx) + 1, 0);
	}

	std::vector<Elem>& operator[](const Idx& idx) {
		return occs[toInt(idx)];
	}

	std::vector<Elem>& lookup(const Idx& idx) {
		if (dirty[toInt(idx)]) {
			clean(idx);
		}
		return occs[toInt(idx)];
	}

	void smudge(const Idx& idx) {
		if (!dirty[toInt(idx)]) {
			dirty[toInt(idx)] = true;
			dirties.push_back(idx);
		}
	}

	void cleanAll() {
		for (Idx dirty : dirties) {
			clean(dirty);
		}
		dirties.clear();
	}

	void clean(const Idx& idx) {
		std::vector<Elem>& vec = occs[toInt(idx)];
		auto end = std::remove_if(vec.begin(), vec.end(), [this](Elem e) {return deleted(e);});
		vec.erase(end, vec.end());
		dirty[toInt(idx)] = false;
	}

	void clear() {
		for (auto& v : occs) {
			v.clear();
		}
		occs.clear();
		dirty.clear();
		dirties.clear();
	}
};

typedef std::vector<Lit> Cl;
typedef std::vector<Cl*> For;

}

// legacy
namespace Glucose {
using Var = Candy::Var;
using Lit = Candy::Lit;
}

// add std::hash template specialization
namespace std {

template<>
struct hash<Candy::Lit> {
	std::size_t operator()(const Candy::Lit& key) const {
		Candy::Var hashedVar = Candy::var(key);
		if (Candy::sign(key) == 0) {
			hashedVar = ~hashedVar;
		}
		return std::hash<Candy::Var>()(hashedVar);
	}
};

}

#endif
