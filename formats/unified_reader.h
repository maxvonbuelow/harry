/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <string>
#include <algorithm>
#include <istream>
#include <fstream>

#include "utils/endian.h"

#ifdef WITH_HRY
#include "hry/reader.h"
#endif
#ifdef WITH_PLY
#include "ply/reader.h"
#endif
#ifdef WITH_OBJ
#include "obj/reader.h"
#endif

namespace unified {
namespace reader {

enum FileType { HRY, PLY, OBJ, UNKNOWN };
FileType get_mesh_type(std::istream &is, const std::string &fn)
{
	std::string ext(fn.end() - 4, fn.end());
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	char magic[4];
	is.read(magic, 4);
	
	FileType type = UNKNOWN;
	if (*((uint32_t*)magic) == htobe32(0xfaffafaf)) type = HRY;
	else if (magic[0] == 'p' && magic[1] == 'l' && magic[2] == 'y') type = PLY;
	else if (ext == ".obj") type = OBJ;

	is.clear();
	is.seekg(0, std::ios::beg);

	if (type != UNKNOWN) return type;

	throw std::runtime_error("Not a mesh file");
}

void read(std::istream &is, const std::string &fn, mesh::Mesh &mesh)
{
	std::string dir = fn.substr(0, fn.find_last_of("/\\"));
	switch (get_mesh_type(is, fn))
	{
#ifdef WITH_HRY
	case HRY:
		hry::reader::read(is, mesh);
		break;
#endif
#ifdef WITH_PLY
	case PLY:
		ply::reader::read(is, mesh);
		break;
#endif
#ifdef WITH_OBJ
	case OBJ:
		obj::reader::read(is, dir, mesh);
		break;
#endif
	default:
		throw std::runtime_error("Currently unimplemented");
	}
}
void read(const std::string &fn, mesh::Mesh &mesh)
{
	std::ifstream is(fn, std::ifstream::binary);
	read(is, fn, mesh);
}

}
}
