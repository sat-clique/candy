/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology
* 
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

#ifndef SRC_CANDY_CORE_STATE_H_
#define SRC_CANDY_CORE_STATE_H_

#include <vector>
#include <limits>
#include <stddef.h>
#include <assert.h>

template <class T, unsigned int N> class State {
private:
	std::vector<T> state;
	T offset;

public:
	State() : state() {
		offset = std::numeric_limits<T>::min();
		clear();
	}

	State(size_t size) : State() {
		state.resize(size, std::numeric_limits<T>::min());
		offset = std::numeric_limits<T>::min();
		clear();
	}

	~State() { }

	void grow() {
		state.push_back(std::numeric_limits<T>::min());
	}

	void grow(size_t size) {
		if (state.size() < size) {
			state.resize(size, std::numeric_limits<T>::min());
		}
	}

	size_t size() {
		return state.size();
	}

	void clear() {
		if (offset < std::numeric_limits<T>::max() - N) {
			offset += N;
		}
		else {
			std::fill(state.begin(), state.end(), std::numeric_limits<T>::min());
			offset = std::numeric_limits<T>::min();
		}
	}

	inline void set(unsigned int index, unsigned int k) {
        assert(index < state.size());
		assert(k < N);
		state[index] = offset + k;
	}

	inline unsigned int get(unsigned int index) const {
        assert(index < state.size());
		return state[index] > offset ? state[index] % N : 0;
	}

    inline unsigned int operator [](unsigned int index) const {
        return get(index);
    }
};

#endif /* SRC_CANDY_CORE_STATE_H_ */
