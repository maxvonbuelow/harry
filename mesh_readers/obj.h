#pragma once

#include <istream>
#include <vector>

#include "../utils.h"
#include "../structs/mesh.h"

namespace reader {
namespace obj {

typedef float real;
static const MixingFmt::Type REALMT = MixingFmt::FLOAT;

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

template <typename T>
struct OBJReader {
	T &handle;
	int attr_lists[3][4]; // VERTEX, TEX, NORMAL

	OBJReader(T &_handle) : handle(_handle),
		attr_lists{ { -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1 } }
	{
	}

	// helper methods
	int init_attr(Attrs attr, int n)
	{
		int &list = attr_lists[attr][n];
		if (list == -1) {
			mixing::Fmt fmt;
			mixing::Interps interps;
			for (int i = 0; i < n; ++i) {
				fmt.add(REALMT);
				interps.append(lut_interp[attr], i);
			}
			list = handle.add_list(fmt, interps);
		}
		return list;
	}
	void write_attr(int list, real *coords, int n)
	{
		int row = handle.alloc_attr(list);
		for (int i = 0; i < n; ++i) {
			unsigned char *dst = handle.elem(list, row, i);
			std::copy(coords + i, coords + i + 1, dst);
		}
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
		int list = init_attr(VERTEX, n);
		write_attr(list, coords, n);
	}
	void tex(real *coords, int n)
	{
	}
	void normal(real *coords, int n)
	{
	}
	void face(bool has_v, int *vi, bool has_t, int *ti, bool has_n, int *ni, int corners)
	{
		if (!has_v) throw std::runtime_error("Face has no vertex index; required for connectivity");
	}

	// reader
	void read_obj(std::istream &is, const std::string &dir)
	{
// 		prog.start(util::linenum_approx(is));

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
// 			prog(line);

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
				std::cout << "Skipping invalid OBJ line " << id << std::endl;
				util::skip_line(is);
			}
		}
// 		prog.end();
	}
};

struct MeshBuilder {
	int attr_as[3][5];
	int vtx_reg[5];
	std::vector<std::pair<ref_t, ref_t>> tex_loc, normal_loc;
	ref_t cur_as_tex, cur_as_normal;
	ref_t cur_idx_tex[5];
	ref_t cur_idx_normal[5];

	vtxidx_t vidx, fidx;
	bool new_region;

	Builder &builder;

	MeshBuilder(Builder &b) : builder(b), vidx(0), fidx(0), attr_as{ { -1 , -1, -1, -1, -1 }, { -1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1 } }, vtx_reg{ -1, -1, -1, -1, -1 }, cur_as_tex(std::numeric_limits<ref_t>::max() - 1), cur_as_normal(std::numeric_limits<ref_t>::max() - 1), cur_idx_tex{ 0, 0, 0, 0, 0 }, cur_idx_normal{ 0, 0, 0, 0, 0 }, new_region(true)
	{
		builder.init_attrs(0, 1, 2);
	}

	ref_t init_attr(Attrs attr, int n, MixingInterp::Interp interpid)
	{
		int as = attr_as[attr][n];
		if (as == -1) {
			as = attr_as[attr][n] = builder.add_attr();
			MixingFmt fmt;
			MixingInterp interp;

			for (int i = 0; i < n; ++i) {
// 				std::cout << "ADDING FMT " << REALMT << std::endl;
				fmt.add(REALMT);
				interp.add("", interpid, i);
			}

			builder.attr(as).init(fmt, interp);
// 			builder.interp(as).print();
		}
// 		attr_loc[attr].push_back(attridx);
		return as;
	}
	void write_attr(ref_t as, real *coords, int n)
	{
// 		std::cout << "wa2" << std::endl;
		MElm elm = builder.attr(as).add();
		for (int i = 0; i < n; ++i) elm.at<real>(i) = coords[i];
		builder.attr(as).finalize(builder.attr(as).backidx());
	}

	void usemtl(const std::string &name)
	{
		new_region = true;
	}
	void smoothgroup(const std::string &name)
	{
		new_region = true;
	}
	void object(const std::string &name)
	{
		new_region = true;
	}
	void group(const std::string &name)
	{
		new_region = true;
	}
	void vertex(real *coords, int n)
	{
		ref_t as = init_attr(VERTEX, n, MixingInterp::POS);
		int r = vtx_reg[n];
		if (r == -1) {
			r = vtx_reg[n] = builder.add_region(0, 1, 0);
			builder.bind_vtx(r, 0, as); // vertex-only region
		}
		builder.add_vtx(r);
		builder.mesh.attrs.attr_vtx(vidx, 0) = vidx; // TODO: IOTA
 
		write_attr(as, coords, n);
		++vidx;
	}
	void tex(real *coords, int n)
	{
		ref_t as = init_attr(TEX, n, MixingInterp::TEX);
		write_attr(as, coords, n);
		tex_loc.push_back(std::make_pair(as, cur_idx_tex[n]++));
	}
	void normal(real *coords, int n)
	{
		ref_t as = init_attr(NORMAL, n, MixingInterp::NORMAL);
		write_attr(as, coords, n);
		normal_loc.push_back(std::make_pair(as, cur_idx_normal[n]++));
	}
	void face(bool has_v, int *vi, bool has_t, int *ti, bool has_n, int *ni, int corners)
	{
// 			std::cout << "F parsed0" << std::endl;
		if (!has_v) throw std::runtime_error("Face has no vertex index; required for connectivity");
// 		std::cout << has_n << " " << has_t << std::endl;
// 		if (has_n) std::cout << ni[0] << std::endl;
// 		if (has_t) std::cout << ti[0] << std::endl;
		ref_t as_tex = has_t ? tex_loc[ti[0]].first : std::numeric_limits<ref_t>::max(), as_normal = has_n ? normal_loc[ni[0]].first : std::numeric_limits<ref_t>::max();
// 			std::cout << "F parsed.5" << std::endl;
// 		ref_t idx_tex = has_t ? tex_loc[ti[0]].second : 0, idx_normal = has_n ? normal_loc[ni[0]].second : 0;
		// check attribute consistency
		for (int i = 1; i < corners; ++i) {
			if (has_t && tex_loc[ti[i]].first != as_tex) throw std::runtime_error("Inconsistent texture attribute types in face");
			if (has_n && normal_loc[ni[i]].first != as_normal) throw std::runtime_error("Inconsistent normal attribute types in face");
		}
// 			std::cout << "F parsed1" << std::endl;

		int num_attrs = (int)has_t + (int)has_n;
		int a_tex = 0, a_normal = (int)has_t;

		// check region consistency
		if (cur_as_tex != as_tex || cur_as_normal != as_normal) { // inf-1: uninitialized; inf: unset (this region does not include tex/normal)
			if (!new_region) {
				if (cur_as_tex == std::numeric_limits<ref_t>::max() - 1) std::cout << "NOTE: Adding initial region" << std::endl;
				else std::cout << "WARNING: Region format inconsistent; adding new region" << std::endl;
			}
			regidx_t r = builder.add_region(0, 0, num_attrs);
			if (has_t) builder.bind_wedge(r, a_tex, as_tex);
			if (has_n) builder.bind_wedge(r, a_normal, as_normal);
			cur_as_tex = as_tex; cur_as_normal = as_normal;
		} else if (new_region) {
// 			std::cout << "WARNING: Skipping unnecessary region" << std::endl;
		}
		new_region = false;
// 			std::cout << "F parsed2" << std::endl;

		// add face
		builder.begin_face(corners);
		for (int i = 0; i < corners; ++i) {
			builder.set_org(vi[i]);
			if (has_t) builder.mesh.attrs.attr_wedge(fidx, i, a_tex) = tex_loc[ti[i]].second;
			if (has_n) builder.mesh.attrs.attr_wedge(fidx, i, a_normal) = normal_loc[ni[i]].second;
		}
		builder.end_face();
		++fidx;
	}
};

template <typename P>
void read(std::istream &is, const std::string &dir, Builder &builder, P &prog)
{
	MtlHandle mtlhandle;
	MeshBuilder objbuilder(builder);
	read_obj(is, dir, objbuilder, mtlhandle, prog);

	Mesh &mesh = builder.mesh;

// 	for (int i = 0; i < mesh.size_vtx(); ++i) {
// 		regidx_t r = mesh.attrs.vtx2reg(i);
// 		std::cout << "Vertex " << i << " (Region " << r << ", num attr: " << mesh.attrs.num_attr_vtx(r) << "):";
// 		for (int j = 0; j < mesh.attrs.num_attr_vtx(r); ++j) {
// 			ref_t as = mesh.attrs.binding_vtx(r, j);
// 			std::cout << " ["; mesh.attrs[as][mesh.attrs.attr_vtx(i, j)].print(std::cout); std::cout << "]";
// 		}
// 		std::cout << std::endl;
// 	}
// 	for (int i = 0; i < mesh.size(); ++i) {
// 		std::cout << "Face " << i << ":";
// 		for (int j = 0; j < mesh.num_edges(i); ++j) {
// 			std::cout << " " << mesh.org(i, j);
// 		}
// 		std::cout << std::endl;
// 	}
}


}
}
