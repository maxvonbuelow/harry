/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <ostream>

namespace cbm {

enum INITOP {
	INIT,
	TRI100, TRI010, TRI001,
	TRI110, TRI101, TRI011,
	TRI111,
	EOM, IFIRST = INIT, ILAST = EOM
};
enum OP { BORDER, CONNBWD, SPLIT, UNION, NM, NEWVTX, CONNFWD, CLOSE, FIRST = BORDER, LAST = CONNFWD }; // close is meta operation; never transmitted

static const char* op2str(OP op)
{
	static const char *lut[] = { "_", "<", "\xE2\x88\x9E", "\xE2\x88\xAA", "~", "*", ">", "?" };
	return lut[op];
}
static const char* iop2str(INITOP iop)
{
	static const char *lut[] = { "\xE2\x96\xB3", "\xE2\x96\xB3\xC2\xB9", "\xE2\x96\xB3\xC2\xB9", "\xE2\x96\xB3\xC2\xB9", "\xE2\x96\xB3\xC2\xB2", "\xE2\x96\xB3\xC2\xB2", "\xE2\x96\xB3\xC2\xB2", "\xE2\x96\xB3\xC2\xB3", "/" };
	return lut[iop];
}

template <typename E>
struct CoderData {
	E a;

	inline CoderData()
	{}

	inline void init(E _a)
	{
		a = _a;
	}
};
template <typename E>
inline std::ostream &operator<<(std::ostream &os, const CoderData<E> &v)
{
	return os << v.a;
}
/*
sequence element edge validness:

seq_.. | frist |  mid  | last
-------+-------+-------+------
e0     |  YES  |       |
e1     |  YES  |  YES  |  YES
e2     |       |       |  YES
*/

}
