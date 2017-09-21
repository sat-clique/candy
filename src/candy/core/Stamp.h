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
	Stamp() {
		stamp = std::numeric_limits<T>::min();
	}

	Stamp(unsigned int size) {
		stamped.resize(size, std::numeric_limits<T>::min());
		stamp = std::numeric_limits<T>::min();
	}

	~Stamp() { }

	void incSize() {
		stamped.push_back(std::numeric_limits<T>::min());
	}

	void incSize(size_t size) {
		if (stamped.size() < size) {
			stamped.resize(size, std::numeric_limits<T>::min());
		}
	}

	void clearStamped() {
		if (stamp < std::numeric_limits<T>::max()) {
			stamp++;
		}
		else {
			std::fill(stamped.begin(), stamped.end(), std::numeric_limits<T>::min());
			stamp = std::numeric_limits<T>::min() + 1;
		}
	}

	void unsetStamped(unsigned int index) {
		stamped[index] = std::numeric_limits<T>::min();
	}

	void setStamped(unsigned int index) {
		stamped[index] = stamp;
	}

	bool isStamped(unsigned int index) {
		return stamped[index] == stamp;
	}
};

#endif /* SRC_CANDY_CORE_STAMP_H_ */
