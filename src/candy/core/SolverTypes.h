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

#ifndef Glucose_SolverTypes_h
#define Glucose_SolverTypes_h

#include <assert.h>
#include <stdint.h>
#include <pthread.h>

#include <vector>
#include <algorithm>
#include <functional>

#include "mtl/Alg.h"
#include "mtl/Alloc.h"

namespace Glucose {
//=================================================================================================
// Variables, literals, lifted booleans, clauses:

// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

typedef int Var;
#define var_Undef (-1)

struct Lit {
	int x;

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

//const Lit lit_Undef = mkLit(var_Undef, false);  // }- Useful special constants.
//const Lit lit_Error = mkLit(var_Undef, true );  // }

const Lit lit_Undef = { -2 };  // }- Useful special constants.
const Lit lit_Error = { -1 };  // }

//=================================================================================================
// Lifted booleans:
//
// NOTE: this implementation is optimized for the case when comparisons between values are mostly
//       between one variable and one constant. Some care had to be taken to make sure that gcc 
//       does enough constant propagation to produce sensible code, and this appears to be somewhat
//       fragile unfortunately.

#define l_True  (Glucose::lbool((uint8_t)0)) // gcc does not do constant propagation if these are real constants.
#define l_False (Glucose::lbool((uint8_t)1))
#define l_Undef (Glucose::lbool((uint8_t)2))

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

//=================================================================================================
// Clause -- a simple class for representing a clause:

class Clause;
typedef RegionAllocator<uint32_t>::Ref CRef;

#define BITS_LBD 13
#define BITS_SIZEWITHOUTSEL 19
#define BITS_REALSIZE 21
class Clause {
	struct {
		unsigned mark :2;
		unsigned learnt :1;
		unsigned szWithoutSelectors : BITS_SIZEWITHOUTSEL;
		unsigned canbedel :1;
		unsigned extra_size :2; // extra size (end of 32bits) 0..3
		unsigned size : BITS_REALSIZE;
		unsigned seen :1;
		unsigned reloced :1;
		unsigned _unused :3; // Unused bits
		unsigned lbd : BITS_LBD;
	} header;

	union {
		Lit lit;
		float act;
		uint32_t abs;
		CRef rel;
	} data[0];

	friend class ClauseAllocator;

	// NOTE: This constructor cannot be used directly (doesn't allocate enough memory).
	template<class V>
	Clause(const V& ps, int _extra_size, bool learnt) {
		assert(_extra_size < (1 << 2));
		header.mark = 0;
		header.learnt = learnt;
		header.extra_size = _extra_size;
		header.reloced = 0;
		header.size = ps.size();
		header.lbd = 0;
		header.canbedel = 1;
		header._unused = 0;
		header.seen = 0;
		for (int i = 0; i < (int) ps.size(); i++)
			data[i].lit = ps[i];

		if (header.extra_size > 0) {
			if (header.learnt)
				data[header.size].act = 0;
			else
				calcAbstraction();
			if (header.extra_size > 1) {
				data[header.size + 1].abs = 0; // learntFrom
			}
		}
	}

public:
	void calcAbstraction() {
		assert(header.extra_size > 0);
		uint32_t abstraction = 0;
		for (int i = 0; i < size(); i++)
			abstraction |= 1 << (var(data[i].lit) & 31);
		data[header.size].abs = abstraction;
	}

	int size() const {
		return header.size;
	}
	void shrink(int i) {
		assert(i <= size());
		if (header.extra_size > 0) {
			data[header.size - i] = data[header.size];
			if (header.extra_size > 1) { // Special case for imported clauses
				data[header.size - i - 1] = data[header.size - 1];
			}
		}
		header.size -= i;
	}
	void pop_back() {
		shrink(1);
	}
	bool learnt() const {
		return header.learnt;
	}
	bool has_extra() const {
		return header.extra_size > 0;
	}
	uint32_t mark() const {
		return header.mark;
	}
	void mark(uint32_t m) {
		header.mark = m;
	}
	const Lit& last() const {
		return data[header.size - 1].lit;
	}

	bool reloced() const {
		return header.reloced;
	}
	CRef relocation() const {
		return data[0].rel;
	}
	void relocate(CRef c) {
		header.reloced = 1;
		data[0].rel = c;
	}

	// NOTE: somewhat unsafe to change the clause in-place! Must manually call 'calcAbstraction' afterwards for
	//       subsumption operations to behave correctly.
	Lit& operator [](int i) {
		return data[i].lit;
	}
	Lit operator [](int i) const {
		return data[i].lit;
	}
	operator const Lit*(void) const {
		return (Lit*) data;
	}

	float& activity() {
		assert(header.extra_size > 0);
		return data[header.size].act;
	}
	uint32_t abstraction() const {
		assert(header.extra_size > 0);
		return data[header.size].abs;
	}

	Lit subsumes(const Clause& other) const;
	void strengthen(Lit p);
	void setLBD(int i) {
		if (i < (1 << (BITS_LBD - 1)))
			header.lbd = i;
		else
			header.lbd = (1 << (BITS_LBD - 1));
	}
	// unsigned int&       lbd    ()              { return header.lbd; }
	unsigned int lbd() const {
		return header.lbd;
	}
	void setCanBeDel(bool b) {
		header.canbedel = b;
	}
	bool canBeDel() {
		return header.canbedel;
	}
	void setSeen(bool b) {
		header.seen = b;
	}
	bool getSeen() {
		return header.seen;
	}
	void setSizeWithoutSelectors(unsigned int n) {
		header.szWithoutSelectors = n;
	}
	unsigned int sizeWithoutSelectors() const {
		return header.szWithoutSelectors;
	}

};

//=================================================================================================
// ClauseAllocator -- a simple class for allocating memory for clauses:

const CRef CRef_Undef = RegionAllocator<uint32_t>::Ref_Undef;
class ClauseAllocator: public RegionAllocator<uint32_t> {
	static int clauseWord32Size(int size, int extra_size) {
		return (sizeof(Clause) + (sizeof(Lit) * (size + extra_size)))
				/ sizeof(uint32_t);
	}
public:
	bool extra_clause_field;

	ClauseAllocator(uint32_t start_cap) :
			RegionAllocator<uint32_t>(start_cap), extra_clause_field(false) {
	}
	ClauseAllocator() :
			extra_clause_field(false) {
	}

	void moveTo(ClauseAllocator& to) {
		to.extra_clause_field = extra_clause_field;
		RegionAllocator<uint32_t>::moveTo(to);
	}

	template<class Lits>
	CRef alloc(const Lits& ps, bool learnt = false) {
		assert(sizeof(Lit) == sizeof(uint32_t));
		assert(sizeof(float) == sizeof(uint32_t));

		bool use_extra = learnt | extra_clause_field;
		int extra_size = use_extra ? 1 : 0;
		CRef cid = RegionAllocator<uint32_t>::alloc(
				clauseWord32Size(ps.size(), extra_size));
		new (lea(cid)) Clause(ps, extra_size, learnt);

		return cid;
	}

	// Deref, Load Effective Address (LEA), Inverse of LEA (AEL):
	Clause& operator[](Ref r) {
		return (Clause&) RegionAllocator<uint32_t>::operator[](r);
	}
	const Clause& operator[](Ref r) const {
		return (Clause&) RegionAllocator<uint32_t>::operator[](r);
	}
	Clause* lea(Ref r) {
		return (Clause*) RegionAllocator<uint32_t>::lea(r);
	}
	const Clause* lea(Ref r) const {
		return (Clause*) RegionAllocator<uint32_t>::lea(r);
	}
	Ref ael(const Clause* t) {
		return RegionAllocator<uint32_t>::ael((uint32_t*) t);
	}

	void free(CRef cid) {
		Clause& c = operator[](cid);
		RegionAllocator<uint32_t>::free(
				clauseWord32Size(c.size(), c.has_extra()));
	}

	void reloc(CRef& cr, ClauseAllocator& to) {
		Clause& c = operator[](cr);

		if (c.reloced()) {
			cr = c.relocation();
			return;
		}

		cr = to.alloc(c, c.learnt());
		c.relocate(cr);

		// Copy extra data-fields:
		// (This could be cleaned-up. Generalize Clause-constructor to be applicable here instead?)
		to[cr].mark(c.mark());
		if (to[cr].learnt()) {
			to[cr].activity() = c.activity();
			to[cr].setLBD(c.lbd());
			to[cr].setSeen(c.getSeen());
			to[cr].setSizeWithoutSelectors(c.sizeWithoutSelectors());
			to[cr].setCanBeDel(c.canBeDel());
		} else if (to[cr].has_extra())
			to[cr].calcAbstraction();
	}
};

//=================================================================================================
// OccLists -- a class for maintaining occurence lists with lazy deletion:

template<class Idx, class Elem, class Deleted>
class OccLists {
	std::vector<std::vector<Elem>> occs;
	std::vector<char> dirty;
	std::vector<Idx> dirties;
	Deleted deleted;

public:
	OccLists() {
	}
	OccLists(const Deleted& d) :
			deleted(d) {
	}

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
		if (dirty[toInt(idx)])
			clean(idx);
		return occs[toInt(idx)];
	}

	void smudge(const Idx& idx) {
		if (dirty[toInt(idx)] == 0) {
			dirty[toInt(idx)] = 1;
			dirties.push_back(idx);
		}
	}

	void cleanAll() {
		for (int i = 0; i < (int) dirties.size(); i++)
			// Dirties may contain duplicates so check here if a variable is already cleaned:
			//if (dirty[toInt(dirties[i])])
			clean(dirties[i]);
		dirties.clear();
	}

	void clean(const Idx& idx) {
		std::vector<Elem>& vec = occs[toInt(idx)];
		auto end = std::remove_if(vec.begin(), vec.end(),
				[this](Elem e) {return deleted(e);});
		vec.erase(end, vec.end());
		dirty[toInt(idx)] = 0;
	}

	void copyTo(OccLists &copy) const {
		copy.clear();
		copy.occs.resize(occs.size());
		for (int i = 0; i < (int) occs.size(); i++)
			copy.occs[i].insert(copy.occs[i].end(), occs[i].begin(),
					occs[i].end());
		copy.dirty.insert(copy.dirty.end(), dirty.begin(), dirty.end());
		copy.dirties.insert(copy.dirties.end(), dirties.begin(), dirties.end());
	}

	void clear() {
		for (std::vector<Elem>& v : occs)
			v.clear();
		occs.clear();
		dirty.clear();
		dirties.clear();
	}
};

/*_________________________________________________________________________________________________
 |
 |  subsumes : (other : const Clause&)  ->  Lit
 |
 |  Description:
 |       Checks if clause subsumes 'other', and at the same time, if it can be used to simplify 'other'
 |       by subsumption resolution.
 |
 |    Result:
 |       lit_Error  - No subsumption or simplification
 |       lit_Undef  - Clause subsumes 'other'
 |       p          - The literal p can be deleted from 'other'
 |________________________________________________________________________________________________@*/
inline Lit Clause::subsumes(const Clause& other) const {
	//if (other.size() < size() || (extra.abst & ~other.extra.abst) != 0)
	//if (other.size() < size() || (!learnt() && !other.learnt() && (extra.abst & ~other.extra.abst) != 0))
	assert(!header.learnt);
	assert(!other.header.learnt);
	assert(header.extra_size > 0);
	assert(other.header.extra_size > 0);
	if (other.header.size < header.size
			|| (data[header.size].abs & ~other.data[other.header.size].abs)
					!= 0)
		return lit_Error;

	Lit ret = lit_Undef;
	const Lit* c = (const Lit*) (*this);
	const Lit* d = (const Lit*) other;

	for (unsigned i = 0; i < header.size; i++) {
		// search for c[i] or ~c[i]
		for (unsigned j = 0; j < other.header.size; j++)
			if (c[i] == d[j])
				goto ok;
			else if (ret == lit_Undef && c[i] == ~d[j]) {
				ret = c[i];
				goto ok;
			}

		// did not find it
		return lit_Error;
		ok: ;
	}

	return ret;
}

inline void Clause::strengthen(Lit p) {
	remove(*this, p);
	calcAbstraction();
}

//=================================================================================================
}

namespace Candy {
using Var = Glucose::Var;
using Lit = Glucose::Lit;
typedef std::vector<Lit> Cl;
typedef std::vector<Cl*> For;
}

// add std::hash template specialization
namespace std {

template<>
struct hash<Candy::Lit> {
	std::size_t operator()(const Candy::Lit& key) const {
		Candy::Var hashedVar = Glucose::var(key);
		if (Glucose::sign(key) == 0) {
			hashedVar = ~hashedVar;
		}
		return std::hash<Candy::Var>()(hashedVar);
	}
};

}

#endif
