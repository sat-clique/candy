/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology
* 
* Credits go to:
* 
* The authors of Glucose: Gilles Audemard, Laurent Simon
* 
* MYFLAG is YOURFLAG
* 
* The idea of the abstraction of the (type-)parameterized stamp stems from Felix Kutzner. 
* Check out his SAT Solver: https://github.com/fkutzner/JamSAT

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

#ifndef SRC_CANDY_CORE_STAMP_H_
#define SRC_CANDY_CORE_STAMP_H_

#include <vector>
#include <limits>
#include <stddef.h>
#include <assert.h>

template <class T> class Stamp {
private:
	std::vector<T> stamped;
	T stamp;

public:
	Stamp() : stamped() {
		stamp = std::numeric_limits<T>::min();
		clear();
	}

	Stamp(size_t size) : Stamp() {
		stamped.resize(size, std::numeric_limits<T>::min());
		stamp = std::numeric_limits<T>::min();
		clear();
	}

	~Stamp() { }

	void grow() {
		stamped.push_back(std::numeric_limits<T>::min());
	}

	void grow(size_t size) {
		if (stamped.size() < size) {
			stamped.resize(size, std::numeric_limits<T>::min());
		}
	}

	size_t size() {
		return stamped.size();
	}

	void clear() {
		if (stamp < std::numeric_limits<T>::max()) {
			stamp++;
		}
		else {
			std::fill(stamped.begin(), stamped.end(), std::numeric_limits<T>::min());
			stamp = std::numeric_limits<T>::min() + 1;
		}
	}

	void unset(unsigned int index) {
        assert(index < stamped.size());
		stamped[index] = std::numeric_limits<T>::min();
	}

	void set(unsigned int index) {
        assert(index < stamped.size());
		stamped[index] = stamp;
	}

    inline bool operator [](unsigned int index) const {
        assert(index < stamped.size());
        return isStamped(index);
    }

	bool isStamped(unsigned int index) const {
		return stamped[index] == stamp;
	}
};

template<>
inline void Stamp<bool>::clear() {
	std::fill(stamped.begin(), stamped.end(), false);
	stamp = true;
}

#endif /* SRC_CANDY_CORE_STAMP_H_ */
