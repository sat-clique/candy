/*
 * Stamp.h
 *
 *  Created on: 14.09.2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_STAMP_H_
#define SRC_CANDY_CORE_STAMP_H_

#include <vector>
#include <limits>
#include <stddef.h>

template <class T> class Stamp {
private:
	std::vector<T> stamped;
	T stamp;

public:
	Stamp() : stamped() {
		stamp = std::numeric_limits<T>::min();
	}

	Stamp(unsigned int size) : Stamp() {
		stamped.resize(size, std::numeric_limits<T>::min());
		stamp = std::numeric_limits<T>::min();
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
