/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "reader.h"

#include "common.h"
#include "../../cbm/decoder.h"
#include "attrcode.h"
#include "io.h"
#include "../../progress.h"

namespace hry {
namespace reader {

struct MeshHandle {
	typedef mesh::conn::fepair Edge;

	mesh::Mesh &mesh;

	MeshHandle(mesh::Mesh &_mesh) : mesh(_mesh)
	{}

	mesh::vtxidx_t num_vtx()
	{
		return mesh.num_vtx();
	}

	inline mesh::faceidx_t add_face(mesh::ledgeidx_t ne)
	{
		return mesh.conn.add_face(ne);
	}
	inline Edge edge(mesh::faceidx_t f)
	{
		return Edge(f, 0);
	}
	inline void set_org(Edge e, mesh::vtxidx_t o)
	{
		mesh.conn.set_org(e, o);
	}
	inline Edge next(Edge e)
	{
		return mesh.conn.enext(e);
	}
	inline void merge(Edge a, Edge b)
	{
		mesh.conn.fmerge(a, b);
	}
};

struct HeaderReader {
	std::istream &is;

	HeaderReader(std::istream &_is) : is(_is)
	{}

	void check_magic()
	{
		uint32_t faffafaf;
		is.read((char*)&faffafaf, sizeof(uint32_t));
		if (faffafaf != htobe32(0xfaffafaf)) throw std::runtime_error("Invalid magic number");
		uint8_t ver[2];
		is.read((char*)ver, 2);
		if (ver[0] != VER_MAJ) throw std::runtime_error(std::string("File format version ") + std::to_string(ver[0]) + "." + std::to_string(ver[1]) + " incompatible to decoder format version " + std::to_string(VER_MAJ) + "." + std::to_string(VER_MIN));
		if (ver[0] == 0 && ver[1] != VER_MIN) throw std::runtime_error(std::string("File format version ") + std::to_string(ver[0]) + "." + std::to_string(ver[1]) + " incompatible to decoder format version " + std::to_string(VER_MAJ) + "." + std::to_string(VER_MIN) + " (All 0.x-versions are incompatible to each other)");
	}

	void read_syntax(mesh::Builder &builder)
	{
		check_magic();
		uint32_t nvfe[3];
		is.read((char*)nvfe, 3 * 4);

		mesh::listidx_t num_bindings_face = 0, num_bindings_vtx = 0, num_bindings_corner = 0;
		std::vector<mesh::attr::Target> targets;

		uint16_t nrfv[2];
		is.read((char*)nrfv, 2 * 2);
		for (mesh::regidx_t r = 0; r < nrfv[0]; ++r) {
			uint16_t nbfc[2];
			is.read((char*)nbfc, 2 * 2);
			builder.add_face_region(nbfc[0], nbfc[1]);
			num_bindings_face = std::max(num_bindings_face, nbfc[0]);
			num_bindings_corner = std::max(num_bindings_corner, nbfc[1]);
			for (mesh::listidx_t a = 0; a < nbfc[0]; ++a) {
				uint16_t b;
				is.read((char*)&b, 2);
				if (b >= targets.size()) targets.resize(b + 1, mesh::attr::NONE);
				targets[b] = mesh::attr::FACE;
				builder.bind_reg_facelist(r, a, b);
			}
			for (mesh::listidx_t a = 0; a < nbfc[1]; ++a) {
				uint16_t b;
				is.read((char*)&b, 2);
				if (b >= targets.size()) targets.resize(b + 1, mesh::attr::NONE);
				targets[b] = mesh::attr::CORNER;
				builder.bind_reg_cornerlist(r, a, b);
			}
		}
		for (mesh::regidx_t r = 0; r < nrfv[1]; ++r) {
			uint16_t nbv;
			is.read((char*)&nbv, 2);
			builder.add_vtx_region(nbv);
			num_bindings_vtx = std::max(num_bindings_vtx, nbv);
			for (mesh::listidx_t a = 0; a < nbv; ++a) {
				uint16_t b;
				is.read((char*)&b, 2);
				if (b >= targets.size()) targets.resize(b + 1, mesh::attr::NONE);
				targets[b] = mesh::attr::VTX;
				builder.bind_reg_vtxlist(r, a, b);
			}
		}

		builder.init_bindings(num_bindings_face, num_bindings_vtx, num_bindings_corner);

		builder.alloc_vtx(nvfe[0]);
		builder.alloc_face(nvfe[1], nvfe[2]);

		for (int i = 0; i < targets.size(); ++i) {
			uint32_t s = 0;
			mixing::Fmt fmt, fmt_dequant;
			mixing::Interps interps;
			if (targets[i] != mesh::attr::NONE) {
				is.read((char*)&s, 4);

				uint16_t nfmt;
				is.read((char*)&nfmt, 2);
				for (int j = 0; j < nfmt; ++j) {
					uint8_t type, quant;
					is.read((char*)&type, 1);
					is.read((char*)&quant, 1);
					fmt.add((mixing::Type)type, quant);
				}

				uint16_t ninterps;
				is.read((char*)&ninterps, 2);
				int off = 0;
				for (int j = 0; j < ninterps; ++j) {
					uint16_t len;
					is.read((char*)&len, 2);
					interps.appendn(j, len, off);
					off += len;
					if (interps.have_name(j)) {
						uint32_t strlen;
						is.read((char*)&strlen, 4);
						std::vector<char> str(strlen);
						is.read((char*)str.data(), strlen);
						std::string name(str.data(), strlen);
						interps.describe(j, name);
					}
				}
			}
			mesh::listidx_t l = builder.add_list(fmt, interps, targets[i]);
			builder.alloc_attr(l, s);
			is.read((char*)builder.mesh.attrs[l].min().data(), builder.mesh.attrs[l].min().bytes()); // TODO
			is.read((char*)builder.mesh.attrs[l].max().data(), builder.mesh.attrs[l].max().bytes()); // TODO
		}

		// read tri types
		uint16_t count;
		is.read((char*)&count, 2);
		for (uint32_t i = 0; i < count; ++i) {
			uint16_t ntri;
			is.read((char*)&ntri, 2);
			builder.seen_edge(ntri);
		}
	}
};

void read(std::istream &is, mesh::Mesh &mesh)
{
	mesh::Builder builder(mesh);
	HeaderReader hr(is);
	hr.read_syntax(builder);

	arith::Decoder<> coder(is);
	HryModels models(builder.mesh);
	io::reader rd(models, coder);
	attrcode::AttrDecoder<io::reader> ac(builder, rd);
	MeshHandle meshhandle(mesh);
	cbm::decode<MeshHandle, io::reader, attrcode::AttrDecoder<io::reader>, mesh::vtxidx_t, mesh::faceidx_t>(meshhandle, rd, ac);
	progress::handle proga;
	ac.decode(proga);
}

}
}
