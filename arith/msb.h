#pragma once

namespace arith {

// http://aggregate.org/MAGIC/#Most%20Significant%201%20Bit
template <typename T>
T msb(T x)
{
	for (int i = 1; i < (sizeof(T) << 3); i <<= 1) {
		x |= (x >> i);
	}
	return x & ~(x >> 1);
}

}
