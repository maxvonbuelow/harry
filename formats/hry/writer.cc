#include "writer.h"

#include <thread>

#include "common.h"
#include "cbm_encoder.h"
#include "io.h"
#include "../../progress.h"

namespace hry {
namespace writer {

struct HeaderWriter {
	std::ostream &os;

	HeaderWriter(std::ostream &_os) : os(_os)
	{}

	void write_magic()
	{
		uint32_t faffafaf = htobe32(0xfaffafaf);
		os.write((char*)&faffafaf, 4);
		uint8_t ver[] = { hry::VER_MAJ, hry::VER_MIN };
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

template <typename D>
void compress(std::ostream &os, mesh::Mesh &mesh, D &draw)
{
	HeaderWriter hw(os);
	hw.write_syntax(mesh);
	os.flush();
	arith::Encoder<> coder(os);
	HryModels models(mesh);
	io::writer wr(models, coder);
	attrcode::AttrCoder<io::writer> ac(mesh, wr);
	progress::handle prog;
	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
	cbm::encode(mesh, draw, wr, ac, prog);
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::cout << "Took: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms" << std::endl;
	progress::handle proga;
	ac.encode(proga);
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::cout << "Took: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms" << std::endl;
	std::cout << "Total: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t0).count() << " ms" << std::endl;
	coder.flush();
}

struct voiddrawer { template <typename ...T> void operator()(T &&...x) {} };

void write(std::ostream &os, mesh::Mesh &mesh)
{
	voiddrawer draw;
	compress(os, mesh, draw);
}

}
}
