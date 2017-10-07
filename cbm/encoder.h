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

#include "cutborder.h"
#include "base.h"

#include "../assert.h"

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

template <typename M, typename W, typename A, typename V = int, typename F = int>
void encode(M &mesh, W &wr, A &ac)
{
	typedef CutBorder<CoderData<typename M::Edge>, V> CutBorder;
	CutBorder cutBorder(mesh.num_vtx());
	typedef typename CutBorder::Data Data;

	V vertexIdx = 0;
	Perm perm(mesh.num_vtx());
	std::vector<uint16_t> order(mesh.num_vtx(), 0);
	F f;

	int curtri, ntri;
	typename M::Edge e0, e1, e2;
	do {
		Data v0, v1, v2, v2op;
		curtri = 0;
		e0 = mesh.choose_tri(); e1 = mesh.next(e0); e2 = mesh.next(e1);
		v0 = Data(mesh.org(e0)); v1 = Data(mesh.org(e1)); v2 = Data(mesh.org(e2));
		bool m0 = perm.isMapped(v0.idx), m1 = perm.isMapped(v1.idx), m2 = perm.isMapped(v2.idx);
		f = mesh.face(e0);
		ntri = mesh.num_edges(f) - 2;
		INITOP initop;
		// Tri 3
		if (m0 && m1 && m2) {
			wr.tri111(ntri, perm.get(v0.idx), perm.get(v1.idx), perm.get(v2.idx));
			initop = TRI111;
		} else if (m0 && m1) {
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
		} else if (m0) {
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
		} else {
			wr.initial(ntri);
			ac.vtx(f, mesh.edge(e0));
			ac.vtx(f, mesh.edge(e1));
			ac.vtx(f, mesh.edge(e2));
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = INIT;
		}
		ac.face(f, mesh.edge(e0));

		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

		v0.init(e0);
		v1.init(e1);
		v2.init(e2);
		cutBorder.initial(v0, v1, v2);

		++curtri;

		while (!cutBorder.atEnd()) {
			cutBorder.traverseStep(v0, v1);
			Data data_gate = v0;
			typename M::Edge gate = v0.a;
			typename M::Edge gateprev = cutBorder.left().a;
			typename M::Edge gatenext = cutBorder.right().a;
			bool seq_first = curtri == ntri;
			bool isvalid = true;
			if (seq_first)
				e0 = mesh.choose_twin(gate, isvalid);

			wr.order(order[v1.idx]);

			if (seq_first && !isvalid) {
				f = mesh.face(gate);
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
					assert_eq(mesh.conn.org(e1), v0.idx);
				} else {
					e1 = mesh.next(e1);
				}
				e2 = mesh.next(e1);
				v2 = Data(mesh.org(e2));

				bool seq_last = curtri + 1 == ntri;

				if (!perm.isMapped(v2.idx)) {
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
						cutBorder.newVertex(v2);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
						wr.nm(seq_first ? ntri : 0, perm.get(v2.idx));
					} else if (op == UNION) {
						wr.cutborderunion(seq_first ? ntri : 0, i, p);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
					} else if (op == CONNFWD || op == CLOSE) {
						if (seq_last && mesh.twin(gatenext) != e2) mesh.merge(gatenext, e2);
						if (op == CLOSE && mesh.twin(gateprev) != e1) mesh.merge(gateprev, e1);
						wr.connectforward(seq_first ? ntri : 0);
						if (op == CONNFWD) cutBorder.first->init(e1);
					} else if (op == CONNBWD) {
						if (mesh.twin(gateprev) != e1) mesh.merge(gateprev, e1);
						wr.connectbackward(seq_first ? ntri : 0);
						if (op == CONNBWD) cutBorder.first->init(e2);
					} else {
						wr.splitcutborder(seq_first ? ntri : 0, i);
						cutBorder.first->init(e1);
						cutBorder.second->init(e2);
					}
				}

				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

				if (seq_first) ac.face(f, mesh.edge(e0));

				++curtri;
			}
		}
	} while (!mesh.empty());
	wr.end();
} 

}
