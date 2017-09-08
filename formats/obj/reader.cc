#include "reader.h"

#include "../../structs/mixing.h"
#include "../../structs/types.h"
#include "../../utils.h"
#include "../../progress.h"

namespace obj {
namespace reader {

typedef float real;
static const mixing::Type REALMT = mixing::FLOAT;
enum Attrs { VERTEX, TEX, NORMAL };

inline int objidx(int n, int size)
{
	if (n == 0) throw std::runtime_error("index cannot be 0");
	if (n > size) throw std::runtime_error("n too big");
	if (n < 0) {
		if (size + n < 0) {
			throw std::runtime_error("n too small");
		}
		return size + n;
	}

	return n - 1;
}

static const int lut_interp[] = { mixing::POS, mixing::TEX, mixing::NORMAL };

#define IL 5 /*std::numeric_limits<mesh::listidx_t>::max()*/
#define IR std::numeric_limits<mesh::regidx_t>::max()

struct OBJReader {
	mesh::Builder &builder;
	mesh::listidx_t attr_lists[3][5]; // VERTEX, TEX, NORMAL
	mesh::regidx_t vtx_reg[5];
	mesh::regidx_t face_reg[64];
	std::vector<std::pair<mesh::listidx_t, mesh::attridx_t>> tex_loc, normal_loc;

	OBJReader(mesh::Builder &_builder) : builder(_builder),
		attr_lists{ { IL, IL, IL, IL, IL }, { IL, IL, IL, IL, IL }, { IL, IL, IL, IL, IL } },
		vtx_reg{ IR, IR, IR, IR, IR },
		face_reg{ IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR,
		          IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR, IR }
	{
	}

	// helper methods
	int init_attr(Attrs attr, int n)
	{
		mesh::listidx_t &list = attr_lists[attr][n];
		if (list == IL) {
			mixing::Fmt fmt;
			mixing::Interps interps;
			for (int i = 0; i < n; ++i) {
				fmt.add(REALMT);
				interps.append(lut_interp[attr], i);
			}
			list = builder.add_list(fmt, interps, attr == VERTEX ? mesh::attr::VTX : mesh::attr::CORNER);
		}
		return list;
	}
	mesh::attridx_t write_attr(int list, real *coords, int n)
	{
		mesh::attridx_t row = builder.alloc_attr(list);
		for (int i = 0; i < n; ++i) {
			unsigned char *dst = builder.elem(list, row, i);
			std::copy((unsigned char*)(coords + i), (unsigned char*)(coords + i + 1), dst);
		}
		return row;
	}

	// OBJ methods
	void usemtl(const std::string &name)
	{
	}
	void smoothgroup(const std::string &name)
	{
	}
	void object(const std::string &name)
	{
	}
	void group(const std::string &name)
	{
	}
	void vertex(real *coords, int n)
	{
		mesh::listidx_t list = init_attr(VERTEX, n);
		mesh::attridx_t aidx = write_attr(list, coords, n);

		mesh::regidx_t r = vtx_reg[n];
		if (r == IR) {
			r = vtx_reg[n] = builder.add_vtx_region(1);
			builder.bind_reg_vtxlist(r, 0, list);
		}
		mesh::vtxidx_t vidx = builder.alloc_vtx();
		builder.vtx_reg(vidx, r);
		builder.bind_vtx_attr(vidx, 0, aidx);
	}
	void tex(real *coords, int n)
	{
		mesh::listidx_t list = init_attr(TEX, n);
		mesh::attridx_t aidx = write_attr(list, coords, n);
		tex_loc.push_back(std::make_pair(list, aidx));
	}
	void normal(real *coords, int n)
	{
		mesh::listidx_t list = init_attr(NORMAL, n);
		mesh::attridx_t aidx = write_attr(list, coords, n);
		normal_loc.push_back(std::make_pair(list, aidx));
	}
	void face(bool has_v, int *vi, bool has_t, int *ti, bool has_n, int *ni, int corners)
	{
		if (!has_v) throw std::runtime_error("Face has no vertex index; required for connectivity");
		mesh::listidx_t tex_l = IL, normal_l = IL;

		if (has_t) tex_l = tex_loc[ti[0]].first;
		if (has_n) normal_l = normal_loc[ni[0]].first;

		for (int c = 1; c < corners; ++c) {
			if (has_t && tex_loc[ti[c]].first != tex_l) throw std::runtime_error("Inconsistent texture attribute types in face");
			if (has_n && normal_loc[ni[c]].first != normal_l) throw std::runtime_error("Inconsistent normal attribute types in face");
		}

		int reglookup = (tex_l << 3) | normal_l;
		mesh::regidx_t r = face_reg[reglookup];
		mesh::listidx_t num_attrs = (mesh::listidx_t)has_t + (mesh::listidx_t)has_n;
		mesh::listidx_t tex_a = 0, normal_a = (mesh::listidx_t)has_t;
		if (r == IR) {
			r = face_reg[reglookup] = builder.add_face_region(0, num_attrs);
			if (has_t) builder.bind_reg_cornerlist(r, tex_a, tex_l);
			if (has_n) builder.bind_reg_cornerlist(r, normal_a, normal_l);
		}

		// add face
		mesh::faceidx_t fidx = builder.alloc_face(1, corners);
		builder.face_reg(fidx, r);
		builder.face_begin(corners);
		for (int i = 0; i < corners; ++i) {
			builder.set_org(vi[i]);
			if (has_t) builder.bind_corner_attr(fidx, i, tex_a, tex_loc[ti[i]].second);
			if (has_n) builder.bind_corner_attr(fidx, i, normal_a, normal_loc[ni[i]].second);
		}
		builder.face_end();
	}

	// reader
	void read_obj(std::istream &is, const std::string &dir)
	{
		progress::handle prog;
		prog.start(util::linenum_approx(is));
		builder.init_bindings(0, 1, 2);

		int vi[3] = { 0 };
		std::vector<int> fi[3];
		std::vector<std::string> mtllibs;
		std::string name;
		real coords[4];

		is >> std::noskipws;

		uint32_t line = 0;
		while (!is.eof()) {
			std::string id;
			util::skip_ws(is);
			if (is.peek() != '\n') is >> id;

			if (!id.empty()) ++line;
			prog(line);

			util::skip_ws(is);
			if (id == "#") {
				util::skip_line(is);
			} else if (id.empty()) {
				// empty line
				util::skip_line(is);
			} else if (id == "mtllib") {
				util::getline(is, name);
				mtllibs.push_back(name);
			} else if (id == "usemtl") {
				util::getline(is, name);
				usemtl(name);
			} else if (id == "s") {
				util::getline(is, name);
				smoothgroup(name);
			} else if (id == "o") {
				util::getline(is, name);
				object(name);
			} else if (id == "g") {
				util::getline(is, name);
				group(name);
			} else if (id == "v") {
				++vi[VERTEX];
				int cnt = util::line2list(is, coords, 4);
				vertex(coords, cnt);
			} else if (id == "vt") {
				++vi[TEX];
				int cnt = util::line2list(is, coords, 3);
				tex(coords, cnt);
			} else if (id == "vn") {
				++vi[NORMAL];
				int cnt = util::line2list(is, coords, 3);
				normal(coords, cnt);
			} else if (id == "vp") {
				util::skip_line(is);
			} else if (id == "f") {
				bool has[3] = { false, false, false };

				for (int i = 0; i < 3; ++i) fi[i].clear();
				while (is.peek() != '\n') {
					int cur;
					util::skip_ws(is);
					for (int i = 0; i < 3; ++i) fi[i].push_back(0);
					for (int i = 0; i < 3; ++i) { // read one */*/* group
						bool empty = is.peek() == '/';
						if (!empty) {
							is >> cur; util::skip_ws(is);
							fi[i].back() = objidx(cur, vi[i]);
							has[i] = true;
						} else {
							has[i] = false;
						}
						if (is.peek() != '/') break;
						else is.get();
					}
				}

				face(has[VERTEX], fi[VERTEX].data(), has[TEX], fi[TEX].data(), has[NORMAL], fi[NORMAL].data(), fi[VERTEX].size());
			} else {
// 				std::cout << "Skipping invalid OBJ line " << id << std::endl;
				util::skip_line(is);
			}
		}
		prog.end();
	}
};

void read(std::istream &is, const std::string &dir, mesh::Mesh &mesh)
{
	mesh::Builder builder(mesh);

	OBJReader reader(builder);
	reader.read_obj(is, dir);

	std::cout << "Used face regions: " << mesh.attrs.num_regs_face() << std::endl;
	std::cout << "Used vertex regions: " << mesh.attrs.num_regs_vtx() << std::endl;

	for (int i = 0; i < mesh.attrs.size(); ++i) {
		std::cout << "Format attr " << i << std::endl;
		for (int j = 0; j < mesh.attrs[i].fmt().size(); ++j) {
			std::cout << mesh.attrs[i].fmt().type(j) << ":" << mesh.attrs[i].fmt().quant(j) << std::endl;
		}
		std::cout << "Interps attr " << i << std::endl;
		for (int j = 0; j < mesh.attrs[i].interps().size(); ++j) {
			std::cout << mesh.attrs[i].interps().off(j) << "." << mesh.attrs[i].interps().len(j) << std::endl;
		}
	}
}

}
}
