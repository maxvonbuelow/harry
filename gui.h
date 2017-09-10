#pragma once

#include "structs/mixing.h"
#include "glwidget.h"
#include "mainwindow.h"
#include "window.h"

struct drawer {
	GLWidget *wid;
	bool firstrecenter;
	mesh::Mesh &mesh;
	std::vector<mesh::listidx_t> reg_pos;

	struct Vertex {
		float d[3];
		Vertex()
		{}
		Vertex(mixing::View v) : d{ v.get<float>(0), v.get<float>(1), v.get<float>(2) } // TODO: interp
		{}
	};
	struct Color {
		float c[3];
		Color()
		{}
		Color(float r, float g, float b) : c{ r, g, b }
		{}
	};

	drawer(GLWidget *w, mesh::Mesh &_mesh) : wid(w), firstrecenter(true), mesh(_mesh)
	{
		// find position stores
		reg_pos.resize(mesh.attrs.num_regs_vtx(), std::numeric_limits<mesh::listidx_t>::max());
		for (int i = 0; i < mesh.attrs.num_regs_vtx(); ++i) {
			for (int j = 0; j < mesh.attrs.num_bindings_vtx_reg(i); ++j) {
				mesh::listidx_t as = mesh.attrs.binding_reg_vtxlist(i, j);
				std::cout << "GUI: Found position data in store " << as << std::endl;
				if (mesh.attrs[as].interps().lens[mixing::POS] >= 3) { reg_pos[i] = j; break; }
			}
		}
	}

	void recenter(Vertex aa, Vertex bb, Vertex cc)
	{
		firstrecenter = false;

		wid->set_center((aa.d[0] + bb.d[0] + cc.d[0]) / 3.0, (aa.d[1] + bb.d[1] + cc.d[1]) / 3.0, (aa.d[2] + bb.d[2] + cc.d[2]) / 3.0);

		float n[3];
		wid->compute_normal(aa.d, bb.d, cc.d, n);
		wid->set_rot_normal(-n[0], -n[1], -n[2]);

		float aabb0[3], aabb1[3];
		float metric = 0;
		for (int i = 0; i < 3; ++i) {
			aabb0[i] = std::min(std::min(aa.d[i], bb.d[i]), cc.d[i]);
			aabb1[i] = std::max(std::max(aa.d[i], bb.d[i]), cc.d[i]);
			metric += aabb1[i] - aabb0[i];
		}

		wid->set_scale(1.0 / metric * 0.2);
	}
	void operator()(mesh::faceidx_t f, int curtri, int ntri, mesh::vtxidx_t a, mesh::vtxidx_t b, mesh::vtxidx_t c, cbm::CutBorderBase::INITOP op)
	{
		(*this)(f, curtri, ntri, a, b, c, (int)op);
	}
	void operator()(mesh::faceidx_t f, int curtri, int ntri, mesh::vtxidx_t a, mesh::vtxidx_t b, mesh::vtxidx_t c, cbm::CutBorderBase::OP op)
	{
		(*this)(f, curtri, ntri, a, b, c, (int)(op + cbm::CutBorderBase::ILAST + 1));
	}

	void operator()(mesh::faceidx_t f, int curtri, int ntri, mesh::vtxidx_t a, mesh::vtxidx_t b, mesh::vtxidx_t c, int op)
	{
		if (curtri == 0) {
			Vertex last, first;
			for (int i = 0; i < mesh.conn.num_edges(f); ++i) {
				mesh::vtxidx_t v = mesh.conn.org(f, i);
				mesh::regidx_t r = mesh.attrs.vtx2reg(v);
				mesh::listidx_t a = reg_pos[r];
				if (a == std::numeric_limits<mesh::listidx_t>::max()) return; // don't draw, because no position attribute found

				mixing::View e = mesh.attrs[mesh.attrs.binding_reg_vtxlist(r, a)][mesh.attrs.binding_vtx_attr(v, a)];
				if (i) {
					wid->push_line(last.d, Vertex(e).d);
				} else {
					first = e;
				}
				last = e;
			}
			wid->push_line(last.d, first.d);
		}
		// triangulate
// 		Vertex first, prev;
// 		for (int i = 0; i < mesh.num_edges(f); ++i) {
// 			mesh::vtxidx_t v = mesh.org(f, i);
// 			mesh::regidx_t r = mesh.attrs.vtx2reg(v);
// 			mesh::attridx_t a = reg_pos[r];
// 			if (a == std::numeric_limits<attridx_t>::max()) return; // don't draw, because no position attribute found
// 			MElm e = mesh.attrs[mesh.attrs.binding_vtx(r, a)][mesh.attrs.attr_vtx(v, a)];
// 			if (i == 0) first = e;
// 			if (i > 1) {
// 				(*this)(first, prev, e, op, i == mesh.num_edges(f) - 1);
// 			}
// 			prev = e;
// 		}
		mesh::regidx_t ra = mesh.attrs.vtx2reg(a), rb = mesh.attrs.vtx2reg(b), rc = mesh.attrs.vtx2reg(c);
		mesh::listidx_t aa = reg_pos[ra], ab = reg_pos[rb], ac = reg_pos[rc];
		if (aa == std::numeric_limits<mesh::listidx_t>::max() || ab == std::numeric_limits<mesh::listidx_t>::max() || ac == std::numeric_limits<mesh::listidx_t>::max()) return; // don't draw, because no position attribute found

		mixing::View ea = mesh.attrs[mesh.attrs.binding_reg_vtxlist(ra, aa)][mesh.attrs.binding_vtx_attr(a, aa)];
		mixing::View eb = mesh.attrs[mesh.attrs.binding_reg_vtxlist(rb, ab)][mesh.attrs.binding_vtx_attr(b, ab)];
		mixing::View ec = mesh.attrs[mesh.attrs.binding_reg_vtxlist(rc, ac)][mesh.attrs.binding_vtx_attr(c, ac)];

		if (curtri != 0) {
			wid->push_linein(Vertex(ea).d, Vertex(eb).d);
		}

		(*this)(ea, eb, ec, op, true);
	}
	void operator()(Vertex aa, Vertex bb, Vertex cc, int op, bool last)
	{
		Color col(1, 1, 1);
		if (op <= cbm::CutBorderBase::ILAST) {
			switch (op) {
			case cbm::CutBorderBase::INIT:
				col = Color(1, 0, 0);
				if (firstrecenter) recenter(aa, bb, cc);
				break;
			case cbm::CutBorderBase::TRI111:
				col = Color(1, 1, 1);
				break;
			case cbm::CutBorderBase::TRI110:
			case cbm::CutBorderBase::TRI101:
			case cbm::CutBorderBase::TRI011:
				col = Color(0.6, 0.6, 0.6);
				break;
			case cbm::CutBorderBase::TRI100:
			case cbm::CutBorderBase::TRI010:
			case cbm::CutBorderBase::TRI001:
				col = Color(0.2, 0.2, 0.2);
				break;
			}
			wid->push_face(aa.d, bb.d, cc.d, col.c);
		} else {
			--op -= (int)cbm::CutBorderBase::ILAST;
			if (op == cbm::CutBorderBase::BORDER) {
				float n[3], e[3], d[3], a2[3], b2[3];
				wid->compute_normal(aa.d, bb.d, cc.d, n);

				for (int i = 0; i < 3; ++i) e[i] = bb.d[i] - aa.d[i];

				d[0] = e[1] * n[2] - e[2] * n[1]; d[1] = e[2] * n[0] - e[0] * n[2]; d[2] = e[0] * n[1] - e[1] * n[0];

				float dl = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
				for (int i = 0; i < 3; ++i) d[i] /= dl;

				float metric = 0.0f;
				for (int i = 0; i < 3; ++i) {
					float aabb0 = std::min(std::min(aa.d[i], bb.d[i]), cc.d[i]);
					float aabb1 = std::max(std::max(aa.d[i], bb.d[i]), cc.d[i]);
					metric += aabb1 - aabb0;
				}
				metric /= 10.0f;
				for (int i = 0; i < 3; ++i) {
					d[i] *= metric;
					a2[i] = aa.d[i] + d[i];
					b2[i] = bb.d[i] + d[i];
				}

				col = Color(0, 1, 1);
				wid->push_face(b2, bb.d, a2, col.c);
				wid->push_face(a2, bb.d, aa.d, col.c);
			} else {
				switch (op) {
				case cbm::CutBorderBase::ADDVTX:
					col = Color(0, 0, 1);
					break;
				case cbm::CutBorderBase::CONNBWD:
					col = Color(0, 1, 0);
					break;
				case cbm::CutBorderBase::CONNFWD:
					col = Color(0, 0.5, 0);
					break;
				case cbm::CutBorderBase::SPLIT:
					col = Color(1, 0, 1);
					break;
				case cbm::CutBorderBase::UNION:
					col = Color(1, 1, 0);
					break;
				case cbm::CutBorderBase::NM:
					col = Color(1, 1, 1);
					break;
				}
				wid->push_face(aa.d, bb.d, cc.d, col.c);
			}
		}

		if (last) {
			// Pause stuff
			while (wid->get_paused()) {
				std::this_thread::sleep_for(10ms);
			}
			int delay = wid->get_delay();
			if (delay > 0) {
				float sizefactor = 1.0 / 100.0;
				std::this_thread::sleep_for(std::chrono::milliseconds((int)(sizefactor * (delay / 10.0) * (delay / 10.0))));
			}
		}
	}
};
