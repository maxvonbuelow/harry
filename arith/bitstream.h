#pragma once

#include <istream>
#include <ostream>

namespace arith {

struct bitistream {
	std::istream &is;
	int idx;
	unsigned char buf;

	bitistream(std::istream &_is) : is(_is), idx(8), buf(0)
	{}

	bitistream &operator>>(unsigned char &bit)
	{
		if (idx == 8) {
			buf = is.get();
			idx = 0;
		}
		bit = (buf >> (7 - idx++)) & 1;
		return *this;
	}
};
struct bitostream {
	std::ostream &os;
	int idx;
	unsigned char buf;

	bitostream(std::ostream &_os) : os(_os), idx(0), buf(0)
	{}

	~bitostream()
	{
		flush();
	}

	void flush()
	{
		if (idx == 0) return; // buffer empty
		os << buf;
		buf = 0; idx = 0; // clear and reset
	}

	bitostream &operator<<(unsigned char bit)
	{
		buf |= bit << (7 - idx);
		if (++idx == 8) {
			flush();
		}
		return *this;
	}
};

}
