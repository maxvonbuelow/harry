#pragma once

#include <vector>

#include "types.h"
#include "faces.h"
#include "mixing.h"

namespace mesh {
namespace attr {

enum Target { FACE, VTX, CORNER };

struct Attr : mixing::Array {
	Target target;
	mixing::Interps interps;

	Attr(const mixing::Fmt &fmt, const mixing::Interps &_interps, Target &_target) : mixing::Array(fmt), interps(_interps), target(_target)
	{}
};

struct Bindings {
	// Assignments face/vtx to region.
	std::vector<regidx_t> face_regs, vtx_regs;
	// Assignments face/vtx/corner (target) index to attribute index. Each target can refer multiple attribute list up to num_bindings_*.
	std::vector<attridx_t> bindings_face_attr, bindings_vtx_attr, bindings_corner_attr;
	listidx_t num_bindings_face, num_bindings_vtx, num_bindings_corner;
	// Assignments region to attribute list index. First a lookup to the offset lists must be done in order to get the offsets in the bindings array.
	std::vector<listidx_t> bindings_reg_facelist, bindings_reg_vtxlist, bindings_reg_cornerlist;
	std::vector<int> off_reg_facelist, off_reg_vtxlist, off_reg_cornerlist;

	Faces &faces;

	Bindings(Faces &_faces) :
		off_reg_facelist(1, 0), off_reg_vtxlist(1, 0), off_reg_cornerlist(1, 0),
		num_bindings_face(0), num_bindings_vtx(0), num_bindings_corner(0),
		faces(_faces)
	{}

	attridx_t &binding_face_attr(faceidx_t f, listidx_t a)
	{
		return bindings_face_attr[f * num_bindings_face + a];
	}
	attridx_t &binding_vtx_attr(vtxidx_t v, listidx_t a)
	{
		return bindings_vtx_attr[v * num_bindings_vtx + a];
	}
	attridx_t &binding_corner_attr(faceidx_t f, ledgeidx_t v, listidx_t a)
	{
		return bindings_corner_attr[(faces.off(f) + v) * num_bindings_corner + a];
	}

	listidx_t &binding_reg_facelist(regidx_t r, listidx_t a)
	{
		return bindings_reg_facelist[off_reg_facelist[r] + a];
	}
	listidx_t &binding_reg_vtxlist(regidx_t r, listidx_t a)
	{
		return bindings_reg_vtxlist[off_reg_vtxlist[r] + a];
	}
	listidx_t &binding_reg_cornerlist(regidx_t r, listidx_t a)
	{
		return bindings_reg_cornerlist[off_reg_cornerlist[r] + a];
	}

	regidx_t num_regs_face() const
	{
		return off_reg_facelist.size() - 1;
	}
	regidx_t num_regs_vtx() const
	{
		return off_reg_vtxlist.size() - 1;
	}

	// actual number of bindings in a region for a face/vtx/corner
	listidx_t num_bindings_face_reg(regidx_t r) const
	{
		return off_reg_facelist[r + 1] - off_reg_facelist[r];
	}
	listidx_t num_bindings_vtx_reg(regidx_t r) const
	{
		return off_reg_vtxlist[r + 1] - off_reg_vtxlist[r];
	}
	listidx_t num_bindings_corner_reg(regidx_t r) const
	{
		return off_reg_cornerlist[r + 1] - off_reg_cornerlist[r];
	}

	faceidx_t num_face() const
	{
		return face_regs.size();
	}
	vtxidx_t num_vtx() const
	{
		return vtx_regs.size();
	}
	edgeidx_t num_edge() const
	{
		return faces.size_edge();
	}

	regidx_t face2reg(faceidx_t f) const
	{
		return face_regs[f];
	}
	regidx_t vtx2reg(vtxidx_t v) const
	{
		return vtx_regs[v];
	}
};

struct Attrs : std::vector<Attr>, Bindings {
	Attrs(Faces &faces) : Bindings(faces)
	{}
};

}
}
