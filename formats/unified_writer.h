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
#include <ostream>
#include <fstream>

#include "hry/writer.h"
#include "ply/writer.h"
#include "obj/writer.h"

namespace unified {
namespace writer {

enum FileType { HRY, PLY, OBJ, UNKNOWN };
FileType get_mesh_type(const std::string &fn)
{
	std::string ext(fn.end() - 4, fn.end());
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	
	FileType type = UNKNOWN;
	if (ext == ".hry") type = HRY;
	else if (ext == ".ply") type = PLY;
	else if (ext == ".obj") type = OBJ;

	if (type != UNKNOWN) return type;

	throw std::runtime_error("Unknown file extension");
}

void write(std::ostream &os, const std::string &fn, mesh::Mesh &mesh, FileType type = UNKNOWN, bool ply_ascii = false)
{
	std::string dir = fn.substr(0, fn.find_last_of("/\\"));
	type = type == UNKNOWN ? get_mesh_type(fn) : type;
	switch (type)
	{
	case HRY:
		hry::writer::write(os, mesh);
		break;
	case PLY:
		ply::writer::write(os, mesh, ply_ascii);
		break;
	case OBJ:
		obj::writer::write(os, dir, mesh);
		break;
	default:
		throw std::runtime_error("Currently unimplemented");
	}
}
void write(const std::string &fn, mesh::Mesh &mesh, FileType type = UNKNOWN, bool ply_ascii = false)
{
	std::ofstream os(fn, std::ofstream::binary);
	write(os, fn, mesh, type, ply_ascii);
}

}
}