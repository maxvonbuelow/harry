#pragma once

#include <vector>
#include <set>
#include <ostream>

#include "../../structs/mixing.h"
#include "../../structs/types.h"

namespace ply {
namespace writer {

inline std::string type2str(mixing::Type t)
{
	switch (t) {
	case mixing::FLOAT:
		return "float";
	case mixing::DOUBLE:
		return "double";
	case mixing::UINT:
		return "uint";
	case mixing::INT:
		return "int";
	case mixing::USHORT:
		return "ushort";
	case mixing::SHORT:
		return "short";
	case mixing::UCHAR:
		return "uchar";
	case mixing::CHAR:
		return "char";
	}
}

static std::vector<std::vector<const char*>> lutinterp2name = {
	{ "x", "y", "z", "w" }, // pos
	{ "nx", "ny", "nz", "nw" }, // normal
	{ "red", "green", "blue" }, // color
	{ "ambient_red", "ambient_green", "ambient_blue" }, // color_ambient
	{ "diffuse_red", "diffuse_green", "diffuse_blue" }, // color_diffuse
	{ "specular_red", "specular_green", "specular_blue" }, // color_specular
	{ "u", "v", "tw" }, // tex
};
static std::vector<const char*> lutinterp2name2 = {
	"pos", "normal", "color", "ambient", "diffuse", "specular", "tex",
};
inline std::string interp2name(const mixing::Interps &interps, int interp, int off)
{
	if (interp < mixing::OTHER) {
		if (interp < lutinterp2name.size()) {
			if (off < lutinterp2name[interp].size()) return lutinterp2name[interp][off];
			return std::string(lutinterp2name2[interp]) + std::string("_") + std::to_string(off);
		}
		return std::string("unknown_") + std::to_string(interp) + std::string("_") + std::to_string(off);
	}
	if (interps.lens[interp] == 1) return interps.names[interp - mixing::OTHER];
	return interps.names[interp] + std::string("_") + std::to_string(off);
}

struct BinWriter {
	void operator()(std::ostream &os, mixing::View v, bool append)
	{
		os.write((char*)v.data(), v.bytes());
	}
	void num_edges(std::ostream &os, char ne)
	{
		os.write((char*)&ne, 1);
	}
	void org(std::ostream &os, uint32_t o)
	{
		os.write((char*)&o, 4);
	}
	void next(std::ostream &os)
	{}
};
struct ASCIIWriter {
	void operator()(std::ostream &os, mixing::View v, bool append)
	{
		v.print(os, append);
	}
	void num_edges(std::ostream &os, char ne)
	{
		os << (int)ne;
	}
	void org(std::ostream &os, uint32_t o)
	{
		os << '\t' << o;
	}
	void next(std::ostream &os)
	{
		os << '\n';
	}
};

template <typename H, typename W>
void writeloop(std::ostream &os, H &handle, const std::vector<bool> &isset_vtx, const std::vector<bool> &isset_face, W &&write)
{
	for (mesh::vtxidx_t i = 0; i < handle.attrs.num_vtx(); ++i) {
		mesh::regidx_t r = handle.attrs.vtx2reg(i);
		for (int j = 0; j < handle.attrs.num_bindings_vtx_reg(r); ++j) {
			mesh::listidx_t as = handle.attrs.binding_reg_vtxlist(r, j);
			if (!isset_vtx[as]) continue;
			write(os, handle.attrs[as][handle.attrs.binding_vtx_attr(i, j)], !!j);
		}
		write.next(os);
	}

	for (mesh::faceidx_t i = 0; i < handle.attrs.num_face(); ++i) {
		mesh::regidx_t r = handle.attrs.face2reg(i);
		uint8_t ne = handle.conn.num_edges(i);
		write.num_edges(os, ne);
		for (int j = 0; j < ne; ++j) {
			uint32_t o = handle.conn.org(i, j);
			write.org(os, o);
		}
		for (int j = 0; j < handle.attrs.num_bindings_face_reg(r); ++j) {
			mesh::listidx_t as = handle.attrs.binding_reg_facelist(r, j);
			if (!isset_face[as]) continue;
			write(os, handle.attrs[as][handle.attrs.binding_face_attr(i, j)], true);
		}
		write.next(os);
	}
}

template <typename H>
void write(std::ostream &os, H &handle, bool ascii = false)
{
	bool le = le16toh(0xff00) == 0xff00; // always write in host byte order :>
	os << "ply\nformat " << (ascii ? "ascii" : le ? "binary_little_endian" : "binary_big_endian") << " 1.0\ncomment decompressed using harry mesh compressor\n";

	std::set<mesh::listidx_t> a, b, c, d;
	std::set<mesh::listidx_t> *as_vtx = &a, *last_vtx = &b, *as_face = &c, *last_face = &d;
	for (int i = 0; i < handle.attrs.num_regs_vtx(); ++i) {
		for (int j = 0; j < handle.attrs.num_bindings_vtx_reg(i); ++j) {
			mesh::listidx_t as = handle.attrs.binding_reg_vtxlist(i, j);
			if (i == 0 || last_vtx->find(as) != last_vtx->end()) as_vtx->insert(as);
		}
		if (handle.attrs.num_bindings_vtx_reg(i)) { std::swap(as_vtx, last_vtx); as_vtx->clear(); }
	}
	for (int i = 0; i < handle.attrs.num_regs_face(); ++i) {
		for (int j = 0; j < handle.attrs.num_bindings_face_reg(i); ++j) {
			mesh::listidx_t as = handle.attrs.binding_reg_facelist(i, j);
			if (i == 0 || last_face->find(as) != last_face->end()) as_face->insert(as);
		}
		if (handle.attrs.num_bindings_face_reg(i)) { std::swap(as_face, last_face); as_face->clear(); }
	}
	std::swap(as_vtx, last_vtx); std::swap(as_face, last_face);

	std::vector<bool> isset_vtx(handle.attrs.size(), false), isset_face(handle.attrs.size(), false);

	os << "element vertex " << handle.attrs.num_vtx() << "\n";
	for (mesh::listidx_t as : *as_vtx) {
		const mixing::Fmt &fmt = handle.attrs[as].fmt();
		const mixing::Interps &interps = handle.attrs[as].interps;
		isset_vtx[as] = true;
		for (int i = 0; i < interps.offs.size(); ++i) {
			for (int j = 0; j < interps.lens[i]; ++j) {
				os << "property " << type2str(fmt.type(interps.offs[i] + j)) << " " << interp2name(interps, i, j) << "\n";
			}
		}
	}

	os << "element face " << handle.attrs.num_face() << "\n";
	os << "property list uchar uint vertex_indices\n";
	for (mesh::listidx_t as : *as_face) {
		const mixing::Fmt &fmt = handle.attrs[as].fmt();
		const mixing::Interps &interps = handle.attrs[as].interps;
		isset_face[as] = true;

		for (int i = 0; i < interps.offs.size(); ++i) {
			for (int j = 0; j < interps.lens[i]; ++j) {
				os << "property " << type2str(fmt.type(interps.offs[i] + j)) << " " << interp2name(interps, i, j) << "\n";
			}
		}
	}
	os << "end_header\n";
	os.flush();

	if (ascii) writeloop(os, handle, isset_vtx, isset_face, ASCIIWriter());
	else writeloop(os, handle, isset_vtx, isset_face, BinWriter());
	os.flush();
}

}
}
