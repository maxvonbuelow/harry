/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "writer.h"

#include "common.h"
#include "attrcode.h"
#include "io.h"
#include "cbm/encoder.h"
#include "utils/progress.h"

namespace hry {
namespace writer {

struct MeshHandle {
	typedef mesh::conn::fepair Edge;

	std::unordered_set<mesh::faceidx_t> remaining_faces;
	mesh::Mesh &mesh;

	inline MeshHandle(mesh::Mesh &_mesh) : mesh(_mesh)
	{
		for (mesh::faceidx_t i = 0; i < mesh.num_face(); ++i) {
			remaining_faces.insert(i);
		}
	}

	mesh::vtxidx_t num_vtx()
	{
		return mesh.num_vtx();
	}

	inline Edge choose_tri()
	{
		mesh::faceidx_t f = remaining_faces.find(0) == remaining_faces.end() ? *remaining_faces.begin() : 0;
		Edge a = mesh::conn::fepair(f, 0);
		remaining_faces.erase(f);
		return a;
	}

	inline Edge choose_twin(Edge i, bool &success)
	{
		mesh::conn::fepair a = mesh.conn.twin(i);
		success = true;
		if (a == i || remaining_faces.find(a.f()) == remaining_faces.end()) {
			success = false;
			return Edge();
		}
		remaining_faces.erase(a.f());
		return a;
	}

	inline bool empty()
	{
		return remaining_faces.empty();
	}

	inline mesh::vtxidx_t org(Edge e)
	{
		return mesh.conn.org(e);
	}
	inline Edge next(Edge e)
	{
		return mesh.conn.enext(e);
	}
	inline Edge twin(Edge e)
	{
		return mesh.conn.twin(e);
	}
	inline void merge(Edge a, Edge b)
	{
		mesh.conn.fmerge(a, b);
	}
	inline void split(Edge e)
	{
		merge(e, e);
	}
	bool border(Edge e)
	{
		return twin(e) == e;
	}

	mesh::ledgeidx_t num_edges(mesh::faceidx_t f)
	{
		return mesh.conn.num_edges(f);
	}
	mesh::faceidx_t face(Edge e)
	{
		return mesh.conn.face(e);
	}
	mesh::ledgeidx_t edge(Edge e)
	{
		return e.e();
	}
};

struct HeaderWriter {
	std::ostream &os;

	HeaderWriter(std::ostream &_os) : os(_os)
	{}

	void write_magic()
	{
		uint32_t faffafaf = htobe32(0xfaffafaf);
		os.write((char*)&faffafaf, 4);
		uint8_t ver[] = { VER_MAJ, VER_MIN };
		os.write((char*)ver, 2);
	}

	void write_syntax(mesh::Mesh &mesh)
	{
		write_magic();
		uint32_t nvfe[] = { mesh.num_vtx(), mesh.num_face(), mesh.num_edge() };
		os.write((const char*)nvfe, 3 * 4);

		// write reg bindings
		std::vector<bool> seen_attrs(mesh.attrs.size(), false);

		uint16_t nrfv[] = { mesh.attrs.num_regs_face(), mesh.attrs.num_regs_vtx() };
		os.write((const char*)nrfv, 2 * 2);
		for (mesh::regidx_t r = 0; r < nrfv[0]; ++r) {
			uint16_t nbfc[] = { mesh.attrs.num_bindings_face_reg(r), mesh.attrs.num_bindings_corner_reg(r) };
			os.write((const char*)nbfc, 2 * 2);
			for (mesh::listidx_t a = 0; a < nbfc[0]; ++a) {
				uint16_t b = mesh.attrs.binding_reg_facelist(r, a);
				seen_attrs[b] = true;
				os.write((const char*)&b, 2);
			}
			for (mesh::listidx_t a = 0; a < nbfc[1]; ++a) {
				uint16_t b = mesh.attrs.binding_reg_cornerlist(r, a);
				seen_attrs[b] = true;
				os.write((const char*)&b, 2);
			}
		}
		for (mesh::regidx_t r = 0; r < nrfv[1]; ++r) {
			uint16_t nbv = mesh.attrs.num_bindings_vtx_reg(r);
			os.write((const char*)&nbv, 2);
			for (mesh::listidx_t a = 0; a < nbv; ++a) {
				uint16_t b = mesh.attrs.binding_reg_vtxlist(r, a);
				seen_attrs[b] = true;
				os.write((const char*)&b, 2);
			}
		}

		// write attribute meta
		for (int i = 0; i < seen_attrs.size(); ++i) {
			if (!seen_attrs[i]) continue;
			uint32_t s = mesh.attrs[i].size();
			os.write((char*)&s, 4);

			const mixing::Fmt &fmt = mesh.attrs[i].fmt();
			uint16_t nfmt = fmt.size();
			os.write((const char*)&nfmt, 2);
			for (int j = 0; j < nfmt; ++j) {
				uint8_t type = fmt.type(j), quant = fmt.quant(j);
				os.write((const char*)&type, 1);
				os.write((const char*)&quant, 1);
			}

			const mixing::Interps &interps = mesh.attrs[i].interps();
			uint16_t ninterps = interps.size();
			os.write((const char*)&ninterps, 2);
			for (int j = 0; j < ninterps; ++j) {
				uint16_t len = interps.len(j);
				os.write((const char*)&len, 2);
				if (interps.have_name(j)) {
					const std::string &name = interps.name(j);
					uint32_t strlen = name.size();
					os.write((const char*)&strlen, 4);
					os.write((const char*)name.data(), strlen);
				}
			}

			os.write((const char*)mesh.attrs[i].min().data(), mesh.attrs[i].min().bytes());
			os.write((const char*)mesh.attrs[i].max().data(), mesh.attrs[i].max().bytes());
		}

		// write tri types
		uint16_t count = 0;
		for (mesh::Faces::EdgeIterator it = mesh.faces.edge_begin(); it != mesh.faces.edge_end(); ++it) {
			++count;
		}
		os.write((char*)&count, 2);
		for (mesh::Faces::EdgeIterator it = mesh.faces.edge_begin(); it != mesh.faces.edge_end(); ++it) {
			uint16_t e = *it;
			os.write((char*)&e, 2);
		}
	}

};

void compress(std::ostream &os, mesh::Mesh &mesh)
{
	HeaderWriter hw(os);
	hw.write_syntax(mesh);
	os.flush();
	arith::Encoder<> coder(os);
	HryModels models(mesh);
	io::writer wr(models, coder);
	attrcode::AttrCoder<io::writer> ac(mesh, wr);
	MeshHandle meshhandle(mesh);
	cbm::encode<MeshHandle, io::writer, attrcode::AttrCoder<io::writer>, mesh::vtxidx_t, mesh::faceidx_t>(meshhandle, wr, ac);
	progress::handle proga;
	ac.encode(proga);
	coder.flush();
}

void write(std::ostream &os, mesh::Mesh &mesh)
{
	compress(os, mesh);
}

}
}
