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

template <typename H, typename T, typename W, typename P>
void encode(H &mesh, T &handle, W &wr, attrcode::AttrCoder<W> &ac, P &prog)
{
	CutBorder<CoderData> cutBorder(mesh.num_vtx());
	typedef CutBorder<CoderData>::Data Data;

	mesh::vtxidx_t vertexIdx = 0;
	mesh::faceidx_t fop;
	Perm perm(mesh.num_vtx());
	std::vector<int> order(mesh.num_vtx(), 0);
	mesh::faceidx_t f;

	prog.start(mesh.num_vtx());
	int nm = 0;
	int curtri, ntri;
	typename H::Edge e0, e1, e2;
	do {
		Data v0, v1, v2, v2op;
		curtri = 0;
		e0 = mesh.choose_tri(); e1 = mesh.next(e0); e2 = mesh.next(e1);
		v0 = Data(mesh.org(e0)); v1 = Data(mesh.org(e1)); v2 = Data(mesh.org(e2));
		bool m0 = perm.isMapped(v0.idx), m1 = perm.isMapped(v1.idx), m2 = perm.isMapped(v2.idx);
		f = mesh.face(e0);
		ntri = mesh.num_edges(f) - 2;
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
			ac.vtx(f, mesh.edge(e2));
			perm.map(v2.idx, vertexIdx++);
			initop = TRI110;
		} else if (m1 && m2) {
			wr.tri011(ntri, perm.get(v1.idx), perm.get(v2.idx));
			ac.vtx(f, mesh.edge(e0));
			perm.map(v0.idx, vertexIdx++);
			initop = TRI011;
		} else if (m2 && m0) {
			wr.tri101(ntri, perm.get(v2.idx), perm.get(v0.idx));
			ac.vtx(f, mesh.edge(e1));
			perm.map(v1.idx, vertexIdx++);
			initop = TRI101;
		}
		// Tri 1
		else if (m0) {
			wr.tri100(ntri, perm.get(v0.idx));
			ac.vtx(f, mesh.edge(e1));
			ac.vtx(f, mesh.edge(e2));
			perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = TRI100;
		} else if (m1) {
			wr.tri010(ntri, perm.get(v1.idx));
			ac.vtx(f, mesh.edge(e2));
			ac.vtx(f, mesh.edge(e0));
			perm.map(v2.idx, vertexIdx++); perm.map(v0.idx, vertexIdx++);
			initop = TRI010;
		} else if (m2) {
			wr.tri001(ntri, perm.get(v2.idx));
			ac.vtx(f, mesh.edge(e0));
			ac.vtx(f, mesh.edge(e1));
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++);
			initop = TRI001;
		}
		// Tri 0 = Initial
		else {
			wr.initial(ntri);
			ac.vtx(f, mesh.edge(e0));
			ac.vtx(f, mesh.edge(e1));
			ac.vtx(f, mesh.edge(e2));
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = INIT;
		}
		assert_eq(mesh.org(e0), v0.idx);
		assert_eq(mesh.org(e1), v1.idx);
		assert_eq(mesh.org(e2), v2.idx);
		ac.face(f, mesh.edge(e0));

		handle(f, curtri, ntri, v0.idx, v1.idx, v2.idx, initop);

		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

		v0.init(e0);
		v1.init(e1);
		v2.init(e2);
		cutBorder.initial(v0, v1, v2);

		++curtri;

		while (!cutBorder.atEnd()) {
			cutBorder.traverseStep(v0, v1);
			Data data_gate = v0;
			typename H::Edge gate = v0.a;
			typename H::Edge gateprev = cutBorder.left().a;
			typename H::Edge gatenext = cutBorder.right().a;
			bool seq_first = curtri == ntri;
			if (seq_first) {
				assert_eq(v0.idx, mesh.org(gate));
// 				assert_eq(v1.idx, mesh.conn.dest(gate));

				e0 = mesh.choose_twin(gate);
			} else {
// 				assert_eq(v0.idx, mesh.conn.dest(e1));
// 				assert_eq(v1.idx, mesh.org(startedge));
// 				e0 = INVALID_PAIR;
			}

			wr.order(order[v1.idx]);

			if (seq_first && e0 == INVALID_PAIR) {
				f = mesh.face(gate);
				handle(f, 0, ntri, v0.idx, v1.idx, mesh.org(mesh.next(mesh.next(gate))), BORDER);
				OP bop = cutBorder.border();

				if (!mesh.border(gate)) mesh.split(gate); // fix bad border

				wr.border(bop);
			} else {
				if (seq_first) {
					curtri = 0;
					f = mesh.face(e0);
					ntri = mesh.num_edges(f) - 2;

					e1 = mesh.next(e0);

					assert_eq(mesh.org(e0), v1.idx);
// 					assert_eq(mesh.conn.dest(e0), v0.idx);
				} else {
					e1 = mesh.next(e1);
				}
				e2 = mesh.next(e1);
				v2 = Data(mesh.org(e2));

				bool seq_last = curtri + 1 == ntri;

				if (!perm.isMapped(v2.idx)) {
					handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, NEWVTX);
					cutBorder.newVertex(v2);
					cutBorder.first->init(e1);
					cutBorder.second->init(e2);
					wr.newvertex(seq_first ? ntri : 0); // TODO
					ac.vtx(f, mesh.edge(e2));
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
						if (seq_last && mesh.twin(gatenext) != e2) mesh.merge(gatenext, e2);
						if (op == CLOSE && mesh.twin(gateprev) != e1) mesh.merge(gateprev, e1);
						wr.connectforward(seq_first ? ntri : 0);
						if (op == CONNFWD) cutBorder.first->init(e1);
					} else if (op == CONNBWD) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CONNBWD);
						if (mesh.twin(gateprev) != e1) mesh.merge(gateprev, e1);
						wr.connectbackward(seq_first ? ntri : 0);
						if (op == CONNBWD) cutBorder.first->init(e2);
					} else {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, SPLIT);
						wr.splitcutborder(seq_first ? ntri : 0, i);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
					}
				}

				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

				if (seq_first) ac.face(f, mesh.edge(e0));

				++curtri;
			}

			prog(vertexIdx);
		}
	} while (!mesh.empty());
	wr.end();
	prog.end();
} 

}
}
