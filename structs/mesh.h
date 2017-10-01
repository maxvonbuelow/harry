/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include "../assert.h"

#include "types.h"
#include "faces.h"
#include "attr.h"
#include "conn.h"

namespace mesh {

struct Mesh {
	conn::Conn conn;
	attr::Attrs attrs;
	Faces faces;

	Mesh() : conn(faces), attrs(faces)
	{}

	// generic
	faceidx_t num_face() const
	{
		return attrs.num_face();
	}
	vtxidx_t num_vtx() const
	{
		return attrs.num_vtx();
	}
	edgeidx_t num_edge() const
	{
		return attrs.num_edge();
	}
};

struct Builder {
	Mesh &mesh;
	conn::Builder builder_conn;

	Builder(Mesh &_mesh) : mesh(_mesh), builder_conn(_mesh.conn)
	{}

	void noautomerge()
	{
		builder_conn.automerge = false;
	}

	// Generic
	vtxidx_t num_vtx()
	{
		return mesh.num_vtx();
	}
	faceidx_t num_face()
	{
		return mesh.num_face();
	}

	// Attributes
	void init_bindings(listidx_t num_bindings_face, listidx_t num_bindings_vtx, listidx_t num_bindings_corner)
	{
		mesh.attrs.num_bindings_face = num_bindings_face;
		mesh.attrs.num_bindings_vtx = num_bindings_vtx;
		mesh.attrs.num_bindings_corner = num_bindings_corner;
	}

	listidx_t add_list(const mixing::Fmt &fmt, const mixing::Interps &interps, mesh::attr::Target target)
	{
		mesh.attrs.emplace_back(fmt, interps, target);
		return mesh.attrs.size() - 1;
	}
	attridx_t alloc_attr(listidx_t al, attridx_t size = 1)
	{
		mesh.attrs[al].resize(mesh.attrs[al].size() + size);
		return mesh.attrs[al].size() - 1;
	}
	faceidx_t alloc_face(faceidx_t size = 1, edgeidx_t size_edges = 0)
	{
		mesh.attrs.face_regs.resize(mesh.attrs.face_regs.size() + size);
		mesh.attrs.bindings_face_attr.resize(mesh.attrs.bindings_face_attr.size() + size * mesh.attrs.num_bindings_face);
		assert(size_edges != 0 || mesh.attrs.num_bindings_corner == 0); // (size_edges == 0) implicates (mesh.attrs.num_bindings_corner == 0)
		mesh.attrs.bindings_corner_attr.resize(mesh.attrs.bindings_corner_attr.size() + size_edges * mesh.attrs.num_bindings_corner);
		return mesh.attrs.face_regs.size() - 1;
	}
	vtxidx_t alloc_vtx(vtxidx_t size = 1)
	{
		mesh.attrs.vtx_regs.resize(mesh.attrs.vtx_regs.size() + size);
		mesh.attrs.bindings_vtx_attr.resize(mesh.attrs.bindings_vtx_attr.size() + size * mesh.attrs.num_bindings_vtx);
		return mesh.attrs.vtx_regs.size() - 1;
	}
	void face_reg(faceidx_t f, regidx_t r)
	{
		mesh.attrs.face_regs[f] = r;
	}
	void vtx_reg(vtxidx_t v, regidx_t r)
	{
		mesh.attrs.vtx_regs[v] = r;
	}
	regidx_t add_face_region(listidx_t num_lists_face, listidx_t num_lists_corner)
	{
		mesh.attrs.off_reg_facelist.push_back(mesh.attrs.off_reg_facelist.back() + num_lists_face);
		mesh.attrs.off_reg_cornerlist.push_back(mesh.attrs.off_reg_cornerlist.back() + num_lists_corner);
		mesh.attrs.bindings_reg_facelist.resize(mesh.attrs.bindings_reg_facelist.size() + num_lists_face);
		mesh.attrs.bindings_reg_cornerlist.resize(mesh.attrs.bindings_reg_cornerlist.size() + num_lists_corner);
		return mesh.attrs.off_reg_facelist.size() - 2;
	}
	regidx_t add_vtx_region(listidx_t num_lists)
	{
		mesh.attrs.off_reg_vtxlist.push_back(mesh.attrs.off_reg_vtxlist.back() + num_lists);
		mesh.attrs.bindings_reg_vtxlist.resize(mesh.attrs.bindings_reg_vtxlist.size() + num_lists);
		return mesh.attrs.off_reg_vtxlist.size() - 2;
	}

	void bind_reg_facelist(regidx_t r, listidx_t a, listidx_t l)
	{
		mesh.attrs.binding_reg_facelist(r, a) = l;
	}
	void bind_reg_vtxlist(regidx_t r, listidx_t a, listidx_t l)
	{
		mesh.attrs.binding_reg_vtxlist(r, a) = l;
	}
	void bind_reg_cornerlist(regidx_t r, listidx_t a, listidx_t l)
	{
		mesh.attrs.binding_reg_cornerlist(r, a) = l;
	}

	void bind_face_attr(faceidx_t f, listidx_t a, attridx_t idx)
	{
		mesh.attrs.binding_face_attr(f, a) = idx;
	}
	void bind_vtx_attr(vtxidx_t v, listidx_t a, attridx_t idx)
	{
		mesh.attrs.binding_vtx_attr(v, a) = idx;
	}
	void bind_corner_attr(faceidx_t f, ledgeidx_t v, listidx_t a, attridx_t idx)
	{
		mesh.attrs.binding_corner_attr(f, v, a) = idx;
	}

	unsigned char *elem(listidx_t l, attridx_t attr, int eidx)
	{
		return mesh.attrs[l][attr].data(eidx);
	}
	void attr_finished(listidx_t l, attridx_t attr)
	{
		mesh.attrs[l].finalize(attr);
	}
	void finished()
	{
		for (listidx_t l = 0; l < mesh.attrs.size(); ++l) {
			mesh.attrs[l].finalize();
		}
	}
	void finished_rev()
	{
		for (listidx_t l = 0; l < mesh.attrs.size(); ++l) {
			mesh.attrs[l].finalize_rev();
		}
	}

	// Connectivity
	void face_begin(ledgeidx_t ne)
	{
		builder_conn.face_begin(ne);
	}
	void face_end()
	{
		builder_conn.face_end();
	}
	void set_org(vtxidx_t v)
	{
		builder_conn.set_org(v);
	}
	void seen_edge(ledgeidx_t ne)
	{
		mesh.faces.seen_edge(ne);
	}
};

}
