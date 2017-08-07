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
				for (int j = 0; j < ninterps; ++j) {
					uint16_t len;
					is.read((char*)&len, 2);
					interps.appendn(j, len);
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

// 		mesh::smalloff_t sizes[3] = { 0 };
// 		for (int i = 0; i < 3; ++i) {
// 			uint32_t s;
// 			is.read((char*)&s, sizeof(uint32_t));
// 			std::vector<mesh::smalloff_t> *vec = &attrs.off_face + i;
// 			vec->resize(s);
// 			is.read((char*)vec->data(), vec->size() * sizeof(mesh::smalloff_t));
// 			for (int j = 0; j < vec->size() - 1; ++j) {
// 				(&attrs.size_face)[i] = std::max((&attrs.size_face)[i], (mesh::smalloff_t)((*vec)[j + 1] - (*vec)[j]));
// 			}
// 			sizes[i] = vec->back();
// 		}
// 
// 		mesh::ref_t max_as = 0;
// 		for (int i = 0; i < 3; ++i) {
// 			std::vector<mesh::ref_t> *vec = &attrs.refs_binding_face + i;
// 			vec->resize(sizes[i]);
// 			is.read((char*)vec->data(), vec->size() * sizeof(mesh::ref_t));
// 			if (!vec->empty()) max_as = std::max(max_as, *std::max_element(vec->begin(), vec->end()));
// 		}
// 
// 		mesh::ref_t num_as = max_as + 1;
// 		attrs.resize(num_as);
// 		for (mesh::ref_t i = 0; i < num_as; ++i) {
// 			uint32_t s;
// 			is.read((char*)&s, sizeof(uint32_t));
// 			MixingFmt fmt(is);
// 			MixingInterp interp(is);
// 			attrs[i].init(fmt, fmt.get_dequantized(), interp);
// 			attrs[i].resize(s);
// 			is.read((char*)attrs[i].min().data(), attrs[i].min().size());
// 			is.read((char*)attrs[i].max().data(), attrs[i].max().size());
// 		}

		// read tri types
		uint32_t count;
		is.read((char*)&count, sizeof(uint32_t));
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t ntri;
			is.read((char*)&ntri, sizeof(uint32_t));
			builder.mesh.faces.seen_edge(ntri);
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
	progress::handle prog;
	decode(handle, rd, prog);
}

}
}
