/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <stdexcept>
#include <vector>
#include <string>
#include <cmath>

#include "reader.h"

#include "structs/mixing.h"
#include "structs/types.h"
#include "structs/quant.h"
#include "utils/io.h"
#include "utils/progress.h"

#define BUFSIZE 16384

%%{
machine ObjParser;
sp = (' ' | '\t')+;
an = [^\r\n]*;

action float_init { sign = 1; val = 0; fraction = 0; denom = 1; exp = 0; expmul = 1; }
action float_sign { sign = fc == '-' ? -1 : 1; }
action float_frac { fraction *= 10; fraction += fc - '0'; denom *= 10; }
action float_int { val *= 10; val += fc - '0'; }
action float_exp { exp *= 10; exp += fc - '0'; }
action float_exp_fin { expmul = std::pow(10.0, exp); }
action float_fin { val += fraction / denom; val *= sign * expmul; }
action float_nan { val = NAN; }
action float_inf { val = INFINITY * sign; }

nan = [nN][aA][nN];
inf = [iI][nN][fF]([iI][nN][iI][tT][yY])?;
exp = ([eE][+\-][0-9]+ $float_exp %float_exp_fin);
float = (
          [+\-]? >float_init $float_sign
          (
            (([0-9]+ $float_int ('.' [0-9]* $float_frac)?) | ('.' [0-9]+ $float_frac)) exp? %float_fin
          ) | inf %float_inf
        ) | nan %float_nan;

idx = '-'? >{ idx_sign = 0; } ${ idx_sign = 1; } [0-9]+ >{ idx = 0; } ${ idx *= 10; idx += fc - '0'; } %{ idx = idx_sign ? -idx : idx; };

idxset =       idx %{ fi[VERTEX].push_back(objidx(idx, vi[VERTEX])); }
         ('/' (idx %{ fi[TEX].push_back(objidx(idx, vi[TEX])); })?)?
         ('/' (idx %{ fi[NORMAL].push_back(objidx(idx, vi[NORMAL])); })?)?;

coord = float %{ coords[ncoord++] = val; };
action initcoord { ncoord = 0; }

eol = sp? '\r'? '\n';

line = ("usemtl" sp an eol) |
       ("mtllib" sp an eol) |
       ("v" (sp coord){3,4} >initcoord (sp coord){3,4}? eol %{ ++vi[VERTEX]; vertex(coords, ncoord); }) |
       ("vt" sp coord >initcoord sp coord (sp coord)? eol %{ ++vi[TEX]; tex(coords, ncoord); }) |
       ("vn" sp coord >initcoord sp coord sp coord eol %{ ++vi[NORMAL]; normal(coords, ncoord); }) |
       ("f" sp ${ fi[0].clear(); fi[1].clear(); fi[2].clear(); } (idxset sp)* idxset sp idxset eol %{ face(fi[VERTEX].data(), !fi[TEX].empty(), fi[TEX].data(), !fi[NORMAL].empty(), fi[NORMAL].data(), fi[VERTEX].size()); }) |
       ("o" sp an eol) | # object
       ("s" sp an eol) | # smooth
       ("g" sp an eol) | # group
       ("l" sp an eol) | # line
       ("p" sp an eol) | # point
       ("#" an eol) |
       ('' eol);

main := (line >{ prog(line++); })*;
}%%

namespace obj {
namespace reader {

%% write data;

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

#define IL 9
#define IR std::numeric_limits<mesh::regidx_t>::max()

struct OBJReader {
	mesh::Builder &builder;
	mesh::listidx_t attr_lists[3][9]; // VERTEX, TEX, NORMAL
	mesh::regidx_t vtx_reg[9];
	mesh::regidx_t face_reg[256];
	std::vector<std::pair<mesh::listidx_t, mesh::attridx_t>> tex_loc, normal_loc;
	std::vector<std::string> mtllibs;

	OBJReader(mesh::Builder &_builder) : builder(_builder),
		attr_lists{ { IL, IL, IL, IL, IL, IL, IL, IL, IL }, { IL, IL, IL, IL, IL, IL, IL, IL, IL }, { IL, IL, IL, IL, IL, IL, IL, IL, IL } },
		vtx_reg{ IR, IR, IR, IR, IR, IR, IR, IR, IR }
	{
		for (int i = 0; i < 256; ++i) {
			face_reg[i] = IR;
		}
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
				int interp = lut_interp[attr];
				if (attr == VERTEX && n > 4)
					interp = n == 8 && i >= 4 || i >= 3 ? mixing::COLOR : interp;
				interps.append(interp, i);
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
	void face(int *vi, bool has_t, int *ti, bool has_n, int *ni, int corners)
	{
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

		double fraction, denom, sign, val, exp, expmul;
		int idx, idx_sign;
		std::vector<int> fi[3];
		real coords[8];
		int ncoord;
		int vi[3] = { 0 };
		int line = 0;

		char buf[BUFSIZE];
		int cs;

		%% write init;

		while (!is.eof()) {
			char *p = buf;
			is.read(p, BUFSIZE);
			char *pe = p + is.gcount();
			char *eof = is.eof() ? pe : nullptr;

			%% write exec;

			if (cs == ObjParser_error) throw std::runtime_error("Unable to parse this OBJ file");
		}
		prog.end();
	}
};

void read(std::istream &is, const std::string &dir, mesh::Mesh &mesh)
{
	mesh::Builder builder(mesh);

	OBJReader reader(builder);
	reader.read_obj(is, dir);

	quant::set_bounds(mesh.attrs);

	std::cout << "Used face regions: " << mesh.attrs.num_regs_face() << std::endl;
	std::cout << "Used vertex regions: " << mesh.attrs.num_regs_vtx() << std::endl;
}

}
}

