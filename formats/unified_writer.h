#pragma once

#include <string>
#include <algorithm>
#include <ostream>
#include <fstream>

#include "ply/writer.h"

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

template <typename H>
void write(std::ostream &os, const std::string &fn, H &handle, FileType type = UNKNOWN)
{
	std::string dir = fn.substr(0, fn.find_last_of("/\\"));
	type = type == UNKNOWN ? get_mesh_type(fn) : type;
	switch (type)
	{
	case PLY:
		ply::writer::write(os, handle);
		break;
// 	case OBJ:
// 		mesh_reader::obj::read(is, dir, builder, prog);
// 		break;
	default:
		throw std::runtime_error("Currently unimplemented");
	}
}
template <typename H>
void write(const std::string &fn, H &handle, FileType type = UNKNOWN)
{
	std::ofstream os(fn, std::ofstream::binary);
	write(os, fn, handle, type);
}

}
}
