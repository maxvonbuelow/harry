/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

/*
 * Implementation of the Cut-Border Machine.
 *
 * Related publications:
 * Gumhold, Stefan, and Wolfgang Straßer. "Real time compression of triangle mesh connectivity." Proceedings of the 25th annual conference on Computer graphics and interactive techniques. ACM, 1998.
 * Gumhold, Stefan. "Improved cut-border machine for triangle mesh compression." Erlangen Workshop. Vol. 99. 1999.
 */

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

namespace hry {
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
void encode(H &mesh, T &handle, W &wr, attrcode::AttrCoder<W> &ac, P &prog)
{
	CutBorder<CoderData> cutBorder(mesh.num_vtx());
	typedef CutBorder<CoderData>::Data Data;
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
		INITOP initop;
		nm += m0 + m1 + m2;
		// Tri 3
		if (m0 && m1 && m2) {
			wr.tri111(ntri, perm.get(v0.idx), perm.get(v1.idx), perm.get(v2.idx));
			initop = TRI111;
		}
		// Tri 2
		else if (m0 && m1) {
			wr.tri110(ntri, perm.get(v0.idx), perm.get(v1.idx));
			ac.vtx(f, e2.e());
			assert_eq(mesh.conn.org(e2), v2.idx);
			perm.map(v2.idx, vertexIdx++);
			initop = TRI110;
		} else if (m1 && m2) {
			wr.tri011(ntri, perm.get(v1.idx), perm.get(v2.idx));
			ac.vtx(f, e0.e());
			assert_eq(mesh.conn.org(e0), v0.idx);
			perm.map(v0.idx, vertexIdx++);
			initop = TRI011;
		} else if (m2 && m0) {
			wr.tri101(ntri, perm.get(v2.idx), perm.get(v0.idx));
			ac.vtx(f, e1.e());
			assert_eq(mesh.conn.org(e1), v1.idx);
			perm.map(v1.idx, vertexIdx++);
			initop = TRI101;
		}
		// Tri 1
		else if (m0) {
			wr.tri100(ntri, perm.get(v0.idx));
			ac.vtx(f, e1.e());
			ac.vtx(f, e2.e());
			assert_eq(mesh.conn.org(e1), v1.idx);
			assert_eq(mesh.conn.org(e2), v2.idx);
			perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = TRI100;
		} else if (m1) {
			wr.tri010(ntri, perm.get(v1.idx));
			ac.vtx(f, e2.e());
			ac.vtx(f, e0.e());
			assert_eq(mesh.conn.org(e2), v2.idx);
			assert_eq(mesh.conn.org(e0), v0.idx);
			perm.map(v2.idx, vertexIdx++); perm.map(v0.idx, vertexIdx++);
			initop = TRI010;
		} else if (m2) {
			wr.tri001(ntri, perm.get(v2.idx));
			ac.vtx(f, e0.e());
			ac.vtx(f, e1.e());
			assert_eq(mesh.conn.org(e0), v0.idx);
			assert_eq(mesh.conn.org(e1), v1.idx);
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++);
			initop = TRI001;
		}
		// Tri 0 = Initial
		else {
			wr.initial(ntri);
			ac.vtx(f, e0.e());
			ac.vtx(f, e1.e());
			ac.vtx(f, e2.e());
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = INIT;
		}
		ac.face(f, e0.e());

		handle(f, curtri, ntri, v0.idx, v1.idx, v2.idx, initop);

		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

		v0.init(e0);
		v1.init(e1);
		v2.init(e2);
		cutBorder.initial(v0, v1, v2);

		startedge = e0; curedge = e1;
		lastfaceedge = e2;
		++curtri;

		while (!cutBorder.atEnd()) {
			cutBorder.traverseStep(v0, v1);
			Data data_gate = v0;
			mesh::conn::fepair gate = v0.a;
			mesh::conn::fepair gateedgeprev = cutBorder.left().a;
			mesh::conn::fepair gateedgenext = cutBorder.right().a;
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
				handle(f, 0, ntri, v0.idx, v1.idx, mesh.conn.org(mesh.conn.enext(mesh.conn.enext(gate))), BORDER);
				f = mesh.conn.face(gate);
				OP bop = cutBorder.border();

				if (mesh.conn.twin(gate) != gate) mesh.conn.fmerge(gate, gate); // fix bad border

				wr.border(bop);
			} else {
				if (seq_first) {
					curtri = 0;
					f = mesh.conn.face(e0);
					ntri = mesh.conn.num_edges(f) - 2;

					e1 = curedge = mesh.conn.enext(e0); e2 = mesh.conn.enext(e1);
					
					assert_eq(mesh.conn.org(e0), v1.idx);
					assert_eq(mesh.conn.dest(e0), v0.idx);
					v2 = Data(mesh.conn.org(e2));
				} else {
					f = mesh.conn.face(startedge); // TODO: not needed

					e1 = curedge = mesh.conn.enext(curedge);
					e2 = mesh.conn.enext(e1);

					v2 = Data(mesh.conn.dest(curedge));
				}
				bool seq_last = curtri + 1 == ntri;
				bool seq_mid = !seq_first && !seq_last;

				if (!perm.isMapped(v2.idx)) {
					handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, NEWVTX);
					cutBorder.newVertex(v2);
					cutBorder.first->init(e1);
					cutBorder.second->init(e2);
					wr.newvertex(seq_first ? ntri : 0); // TODO
					ac.vtx(f, e2.e());
					perm.map(v2.idx, vertexIdx++);
				} else {
					int i, p;
					OP op;
					bool succ = cutBorder.findAndUpdate(v2, i, p, op);
					if (!succ) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, NM);
						cutBorder.newVertex(v2);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
						wr.nm(seq_first ? ntri : 0, perm.get(v2.idx));
						++nm;
					} else if (op == UNION) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, UNION);
						wr.cutborderunion(seq_first ? ntri : 0, i, p);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
					} else if (op == CONNFWD || op == CLOSE) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CONNFWD);
						if (seq_last && mesh.conn.twin(gateedgenext) != e2) mesh.conn.fmerge(gateedgenext, e2);
						if (op == CLOSE && mesh.conn.twin(gateedgeprev) != e1) mesh.conn.fmerge(gateedgeprev, e1);
						wr.connectforward(seq_first ? ntri : 0);
						if (op == CONNFWD) cutBorder.first->init(e1);
					} else if (op == CONNBWD) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CONNBWD);
						if (mesh.conn.twin(gateedgeprev) != e1) mesh.conn.fmerge(gateedgeprev, e1);
						wr.connectbackward(seq_first ? ntri : 0);
						if (op == CONNBWD) cutBorder.first->init(e2);
					} else {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, SPLIT);
						wr.splitcutborder(seq_first ? ntri : 0, i);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
					}
					assert_eq(curtri + 1 == ntri, seq_last); 
				}

				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];


				if (seq_first) ac.face(f, e0.e());

				++curtri;
			}

			prog(vertexIdx);
		}
	} while (!tri.empty());
	wr.end();
	prog.end();
} 

}
}
