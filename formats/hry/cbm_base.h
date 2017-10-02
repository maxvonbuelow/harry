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
#include "../../structs/mesh.h"

#define INVALID_PAIR mesh::conn::fepair()

namespace hry {
namespace cbm {

struct CoderData {
	mesh::conn::fepair a;

	inline CoderData()
	{}

	inline void init(mesh::conn::fepair _a)
	{
		a = _a;
	}
};
inline std::ostream &operator<<(std::ostream &os, const CoderData &v)
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

struct CBMStats {
	int used_parts, used_elements;
	int nm;
};

}
}
