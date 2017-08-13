#pragma once

#include <istream>
#include "cutbordermashine.h"
#include "io.h"
#include "../../progress.h"

namespace hry {
namespace reader {

struct HeaderReader {
	std::istream &is;

	HeaderReader(std::istream &_is) : is(_is)
	{}

	void check_magic()
	{
		uint32_t faffafaf;
		is.read((char*)&faffafaf, sizeof(uint32_t));
		if (faffafaf != htobe32(0xfaffafaf)) throw std::runtime_error("Invalid magic number");
	}

	template <typename H>
	void read_syntax(H &builder)
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
			mixing::Fmt fmt;
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
			builder.alloc_attr(builder.add_list(fmt, interps, targets[i]), s);
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

template <typename H>
void read(std::istream &is, H &handle)
{
	HeaderReader hr(is);
	hr.read_syntax(handle);

	arith::Decoder<> coder(is);
	HryModels models(handle.mesh);
	::reader rd(models, coder);
	attrcode::AttrDecoder<::reader> ac(handle, rd);
	progress::handle prog;
	decode(handle, rd, ac, prog);
	progress::handle proga;
	ac.decode(proga);
}

}
}
