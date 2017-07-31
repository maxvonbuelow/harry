#pragma once

#include "ply.h"
#include "obj.h"

namespace mesh_reader {
enum FileType { T_PLY, T_OBJ, T_OFF };
FileType get_mesh_type(std::istream &is, const std::string &fn)
{
	std::string ext(fn.end() - 4, fn.end());
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".obj") return T_OBJ;

	std::string magic;
	is >> magic;
	if (magic == "ply") return T_PLY;
	if (magic[magic.size() - 3] == 'O' && magic[magic.size() - 2] == 'F' && magic[magic.size() - 1] == 'F') return T_OFF;

	throw std::runtime_error("Not a mesh file");
}

template <typename P>
void load_mesh(std::istream &is, const std::string &fn, mesh::Builder &builder, P &prog)
{
	std::string dir = fn.substr(0, fn.find_last_of("/\\"));
	switch (get_mesh_type(is, fn))
	{
	case T_PLY:
		mesh_reader::ply::read(is, builder, prog);
		break;
	case T_OBJ:
		mesh_reader::obj::read(is, dir, builder, prog);
		break;
	case T_OFF:
		// TODO
// 		break;
	default:
		throw std::runtime_error("Currently unimplemented");
		break;
	}
}
}
