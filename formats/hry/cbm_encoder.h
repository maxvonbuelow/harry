#pragma once

#include <unordered_set>
#include <vector>
#include <cmath>
#include <iostream>

#include "cutborder.h"
#include "cbm_base.h"
#include "io.h"

#include "../../assert.h"

#include "attrcode.h"

namespace cbm {

struct Perm {
	std::vector<int> perm;

	inline Perm(int n) : perm(n, -1)
	{}

	inline bool isMapped(int idx)
	{
		return perm[idx] != -1;
	}

	inline void map(int a, int b)
	{
		perm[a] = b;
	}
	inline void unmap(int a)
	{
		perm[a] = -1;
	}
	inline int get(int idx)
	{
		return perm[idx];
	}
};

struct Triangulator {
	std::unordered_set<mesh::faceidx_t> remaining_faces;
	const mesh::conn::Conn &conn;

	inline Triangulator(const mesh::Mesh &mesh) : conn(mesh.conn)
	{
		for (mesh::faceidx_t i = 0; i < mesh.num_face(); ++i) {
			remaining_faces.insert(i);
		}
	}

	inline mesh::conn::fepair chooseTriangle()
	{
		mesh::faceidx_t f = remaining_faces.find(0) == remaining_faces.end() ? *remaining_faces.begin() : 0;
		mesh::conn::fepair a = mesh::conn::fepair(f, 0);
		remaining_faces.erase(f);
		return a;
	}

	inline mesh::conn::fepair getVertexData(mesh::conn::fepair i)
	{
		mesh::conn::fepair a = conn.twin(i);
		if (a == i) return INVALID_PAIR;
		if (remaining_faces.find(a.f()) == remaining_faces.end()) return INVALID_PAIR; // this can happen on non manifold edges
		remaining_faces.erase(a.f());
		return a;
	}

	inline bool empty()
	{
		return remaining_faces.empty();
	}
};


template <typename H, typename T, typename W, typename P>
CBMStats encode(H &mesh, T &handle, W &wr, attrcode::AttrCoder<W> &ac, P &prog)
{

	typedef DataTpl<CoderData> Data;
	typedef ElementTpl<CoderData> Element;
	CutBorder<CoderData> cutBorder(100 + 20000, 10 * std::sqrt(mesh.num_vtx()) + 10000000); // TODO: wrong assumption in CBM paper
	Triangulator tri(mesh);

	mesh::vtxidx_t vertexIdx = 0;
	mesh::faceidx_t fop;
	Perm perm(mesh.num_vtx());
	std::vector<int> order(mesh.num_vtx(), 0);
	mesh::faceidx_t f;

	prog.start(mesh.num_vtx());
	int nm = 0;
	int curtri, ntri;
	mesh::conn::fepair curedge, startedge, lastfaceedge;
	mesh::conn::fepair e0, e1, e2;
	do {
		Data v0, v1, v2, v2op;
		curtri = 0;
		e0 = tri.chooseTriangle(); e1 = mesh.conn.enext(e0); e2 = mesh.conn.enext(e1);
		v0 = Data(mesh.conn.org(e0)); v1 = Data(mesh.conn.org(e1)); v2 = Data(mesh.conn.org(e2));
		bool m0 = perm.isMapped(v0.idx), m1 = perm.isMapped(v1.idx), m2 = perm.isMapped(v2.idx);
		f = mesh.conn.face(e0);
		ntri = mesh.conn.num_edges(f) - 2;
		CutBorderBase::INITOP initop;
		nm += m0 + m1 + m2;
		// Tri 3
		if (m0 && m1 && m2) {
			wr.tri111(ntri, perm.get(v0.idx), perm.get(v1.idx), perm.get(v2.idx));
// 			ac.face(f);
// 			ac.vtx(f, e0.e());
			initop = CutBorderBase::TRI111;
		}
		// Tri 2
		else if (m0 && m1) {
			wr.tri110(ntri, perm.get(v0.idx), perm.get(v1.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e2.e());
			assert_eq(mesh.conn.org(e2), v2.idx);
			perm.map(v2.idx, vertexIdx++);
			initop = CutBorderBase::TRI110;
		} else if (m1 && m2) {
			wr.tri011(ntri, perm.get(v1.idx), perm.get(v2.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e0.e());
			assert_eq(mesh.conn.org(e0), v0.idx);
			perm.map(v0.idx, vertexIdx++);
			initop = CutBorderBase::TRI011;
		} else if (m2 && m0) {
			wr.tri101(ntri, perm.get(v2.idx), perm.get(v0.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e1.e());
			assert_eq(mesh.conn.org(e1), v1.idx);
			perm.map(v1.idx, vertexIdx++);
			initop = CutBorderBase::TRI101;
		}
		// Tri 1
		else if (m0) {
			wr.tri100(ntri, perm.get(v0.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e1.e());
			ac.vtx(f, e2.e());
			assert_eq(mesh.conn.org(e1), v1.idx);
			assert_eq(mesh.conn.org(e2), v2.idx);
			perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = CutBorderBase::TRI100;
		} else if (m1) {
			wr.tri010(ntri, perm.get(v1.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e2.e());
			ac.vtx(f, e0.e());
			assert_eq(mesh.conn.org(e2), v2.idx);
			assert_eq(mesh.conn.org(e0), v0.idx);
			perm.map(v2.idx, vertexIdx++); perm.map(v0.idx, vertexIdx++);
			initop = CutBorderBase::TRI010;
		} else if (m2) {
			wr.tri001(ntri, perm.get(v2.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e0.e());
			ac.vtx(f, e1.e());
			assert_eq(mesh.conn.org(e0), v0.idx);
			assert_eq(mesh.conn.org(e1), v1.idx);
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++);
			initop = CutBorderBase::TRI001;
		}
		// Tri 0 = Initial
		else {
			wr.initial(ntri);
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e0.e());
			ac.vtx(f, e1.e());
			ac.vtx(f, e2.e());
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = CutBorderBase::INIT;
		}
		ac.face(f, e0.e());

		handle(f, curtri, ntri, v0.idx, v1.idx, v2.idx, initop);

		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

// 		ac.wedge(f, 0, v0.idx); ac.wedge(f, 1, v1.idx); ac.wedge(f, 2, v2.idx);

		v0.init(e0);
		v1.init(e1);
		v2.init(e2);
		cutBorder.initial(v0, v1, v2);
// 		cutBorder.init(cbe0->next->next, ntri == 1 ? e2 : INVALID_PAIR, v1.idx);

		startedge = e0; curedge = e1;
		lastfaceedge = e2;
		++curtri;

		while (!cutBorder.atEnd()) {
			Element *elm_gate = cutBorder.traverseStep(v0, v1);
			Data *data_gate = &elm_gate->data;
			mesh::conn::fepair gate = data_gate->a;
			mesh::conn::fepair gateedgeprev = elm_gate->prev->data.a;
			mesh::conn::fepair gateedgenext = elm_gate->next->data.a;
			bool seq_first = curtri == ntri;
			if (seq_first) {
				lastfaceedge = gate;

				assert_eq(v0.idx, mesh.conn.org(gate));
				assert_eq(v1.idx, mesh.conn.dest(gate));

				e0 = tri.getVertexData(gate);
				startedge = e0;
			} else {
				assert_eq(v0.idx, mesh.conn.dest(curedge));
				assert_eq(v1.idx, mesh.conn.org(startedge));
				e0 = INVALID_PAIR;
			}

			wr.order(order[v1.idx]);

			if (seq_first && e0 == INVALID_PAIR) {
				handle(f, 0, ntri, v0.idx, v1.idx, mesh.conn.org(mesh.conn.enext(mesh.conn.enext(gate))), CutBorderBase::BORDER);
				f = mesh.conn.face(gate);
				CutBorderBase::OP bop = cutBorder.border();

				if (mesh.conn.twin(gate) != gate) mesh.conn.swap(gate, gate); // fix bad border

// 				bop = CutBorderBase::BORDER;
				wr.border(bop);
			} else {
				if (seq_first) {
					curtri = 0;
					f = mesh.conn.face(e0);
					fop = mesh.conn.face(gate);
					ntri = mesh.conn.num_edges(f) - 2;

					e1 = curedge = mesh.conn.enext(e0); e2 = mesh.conn.enext(e1);
					
					assert_eq(mesh.conn.org(e0), v1.idx);
					assert_eq(mesh.conn.dest(e0), v0.idx);
					v2 = Data(mesh.conn.org(e2));
				} else {
					f = mesh.conn.face(startedge); // TODO: not needed
					fop = mesh.conn.face(lastfaceedge);

					e1 = curedge = mesh.conn.enext(curedge);
					e2 = mesh.conn.enext(e1);

					v2 = Data(mesh.conn.dest(curedge));
				}
				bool seq_last = curtri + 1 == ntri;
				bool seq_mid = !seq_first && !seq_last;

				if (!perm.isMapped(v2.idx)) {
					handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::ADDVTX);
					cutBorder.newVertex(v2);
					cutBorder.last->init(e2);
					data_gate->init(e1);
					wr.newvertex(seq_first ? ntri : 0); // TODO
					ac.vtx(f, e2.e());
					perm.map(v2.idx, vertexIdx++);
				} else {
					int i, p;
					CutBorderBase::OP op;
					bool succ = cutBorder.findAndUpdate(v2, i, p, op);
					if (!succ) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::NM);
						cutBorder.newVertex(v2);
						cutBorder.last->init(e2);
						data_gate->init(e1);
						wr.nm(seq_first ? ntri : 0, perm.get(v2.idx));
						++nm;
// 						e0 = mesh::Mesh::INVALID_PAIR;
					} else if (op == CutBorderBase::UNION) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::UNION);
						wr.cutborderunion(seq_first ? ntri : 0, i, p);
						cutBorder.last->init(e2);
						data_gate->init(e1);
					} else if (op == CutBorderBase::CONNFWD || op == CutBorderBase::CLOSEFWD) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::CONNFWD);
						if (seq_last && mesh.conn.twin(gateedgenext) != e2) mesh.conn.swap(gateedgenext, e2);
						if (op == CutBorderBase::CLOSEFWD && mesh.conn.twin(gateedgeprev) != e1) mesh.conn.swap(gateedgeprev, e1);
						wr.connectforward(seq_first ? ntri : 0);
						data_gate->init(e1);
					} else if (op == CutBorderBase::CONNBWD || op == CutBorderBase::CLOSEBWD) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::CONNBWD);
						if (mesh.conn.twin(gateedgeprev) != e1) mesh.conn.swap(gateedgeprev, e1);
						if (seq_last && op == CutBorderBase::CLOSEBWD && mesh.conn.twin(gateedgenext) != e2) mesh.conn.swap(gateedgenext, e2);
						wr.connectbackward(seq_first ? ntri : 0);
						data_gate->init(e2);
					} else {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::SPLIT);
						wr.splitcutborder(seq_first ? ntri : 0, i);
						cutBorder.last->init(e2);
						data_gate->init(e1);
					}
					assert_eq(curtri + 1 == ntri, seq_last); 
					if (seq_last) cutBorder.preserveOrder();
				}

				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];


				if (seq_first) {
					ac.face(f, e0.e());
// 					ac.wedge(f, 0, v1.idx); ac.wedge(f, 1, v0.idx); ac.wedge(f, 2, v2.idx);
				} else {
// 					ac.wedge(f, curtri + 2, /* mesh.org(f, curtri + 2)*/v2.idx);
				}

				++curtri;
			}

			prog(vertexIdx);
		}
	} while (!tri.empty());
	wr.end();
	prog.end();

	return CBMStats{ cutBorder.max_parts, cutBorder.max_elements, nm };
} 

}
