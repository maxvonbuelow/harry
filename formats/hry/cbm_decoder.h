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

#include <vector>

#include "cutborder.h"
#include "cbm_base.h"

#include "../../assert.h"

#include "attrcode.h"

namespace hry {
namespace cbm {

template <typename H, typename R, typename P, typename V = int, typename F = int>
void decode(H &mesh, R &rd, attrcode::AttrDecoder<R> &ac, P &prog)
{
	CutBorder<CoderData, V> cutBorder(mesh.num_vtx());
	typedef typename CutBorder<CoderData, V>::Data Data;
	std::vector<uint16_t> order(mesh.num_vtx(), 0);

	V vertexIdx = 0;
	F f;
	int i, p;

	prog.start(mesh.num_vtx());

	int curtri, ntri;
	typename H::Edge e0, e1, e2;
	do {
		Data v0, v1, v2;
		INITOP initop = rd.iop();
		if (initop == EOM) break;
		curtri = 0;
		switch (initop) {
		case INIT:
			v0.idx = vertexIdx++; v1.idx = vertexIdx++; v2.idx = vertexIdx++;
			break;
		case TRI100:
			v0.idx = rd.vertid(); v1.idx = vertexIdx++; v2.idx = vertexIdx++;
			break;
		case TRI010:
			v2.idx = vertexIdx++; v1.idx = rd.vertid(); v0.idx = vertexIdx++;
			break;
		case TRI001:
			v0.idx = vertexIdx++; v1.idx = vertexIdx++; v2.idx = rd.vertid();
			break;
		case TRI110:
			v0.idx = rd.vertid(); v1.idx = rd.vertid(); v2.idx = vertexIdx++;
			break;
		case TRI101:
			v2.idx = rd.vertid(); v1.idx = vertexIdx++; v0.idx = rd.vertid();
			break;
		case TRI011:
			v0.idx = vertexIdx++; v1.idx = rd.vertid(); v2.idx = rd.vertid();
			break;
		case TRI111:
			v0.idx = rd.vertid(); v1.idx = rd.vertid(); v2.idx = rd.vertid();
			break;
		}
		ntri = rd.numtri();
		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

		f = mesh.add_face(ntri + 2);
		e0 = mesh.edge(f); e1 = mesh.next(e0); e2 = mesh.next(e1);
		mesh.set_org(e0, v0.idx); mesh.set_org(e1, v1.idx); mesh.set_org(e2, v2.idx);

		++curtri;

		switch (initop) {
		case INIT:
			ac.vtx(f, 0); ac.vtx(f, 1); ac.vtx(f, 2);
			break;
		case TRI100:
			ac.vtx(f, 1); ac.vtx(f, 2);
			break;
		case TRI010:
			ac.vtx(f, 2); ac.vtx(f, 0);
			break;
		case TRI001:
			ac.vtx(f, 0); ac.vtx(f, 1);
			break;
		case TRI110:
			ac.vtx(f, 2);
			break;
		case TRI101:
			ac.vtx(f, 1);
			break;
		case TRI011:
			ac.vtx(f, 0);
			break;
		case TRI111:
			break;
		}
		ac.face(f, 0);

		v0.init(e0);
		v1.init(e1);
		v2.init(e2);

		cutBorder.initial(v0, v1, v2);

		if (curtri == ntri) ++f;

		while (!cutBorder.atEnd()) {
			cutBorder.traverseStep(v0, v1);
			typename H::Edge gate = v0.a;
			typename H::Edge gateprev = cutBorder.left().a;
			typename H::Edge gatenext = cutBorder.right().a;

			rd.order(order[v1.idx]);

			OP op = rd.op(), realop = op;

			bool seq_first = curtri == ntri;

			switch (op) {
			case CONNFWD:
				v2 = cutBorder.connectForward(realop);
				break;
			case CONNBWD:
				v2 = cutBorder.connectBackward(realop);
				break;
			case SPLIT:
				i = rd.elem();
				v2 = cutBorder.splitCutBorder(i);
				break;
			case UNION:
				i = rd.elem();
				p = rd.part();
				v2 = cutBorder.cutBorderUnion(i, p);
				break;
			case NEWVTX:
				v2 = Data(vertexIdx++);
				cutBorder.newVertex(v2);
				break;
			case NM:
				v2.idx = rd.vertid();
				cutBorder.newVertex(v2);
				break;
			case BORDER:
				cutBorder.border();
				v2 = Data(-1);
				break;
			}

			if (!v2.isUndefined()) {
				if (seq_first) {
					ntri = rd.numtri();
					curtri = 0;
					f = mesh.add_face(ntri + 2);
					e0 = mesh.edge(f); e1 = mesh.next(e0); e2 = mesh.next(e1);
					mesh.set_org(e0, v1.idx); mesh.set_org(e1, v0.idx); mesh.set_org(e2, v2.idx);
				} else {
					e1 = mesh.next(e1);
					e2 = mesh.next(e1);
					mesh.set_org(e2, v2.idx);
				}
				bool seq_last = curtri + 1 == ntri;

				switch (realop) {
				case CONNFWD:
					cutBorder.first->init(e1);
					break;
				case CONNBWD:
					cutBorder.first->init(e2);
					break;
				case SPLIT:
				case UNION:
				case NEWVTX:
				case NM:
					cutBorder.first->init(e1);
					cutBorder.second->init(e2);
					break;
				}

				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

				if (op == NEWVTX) ac.vtx(f, curtri + 2);
				if (seq_first) ac.face(f, 0);
				++curtri;

				if (seq_first) mesh.merge(gate, e0);

				switch (op) {
				case CONNFWD:
					if (seq_last && realop != BORDER)
						mesh.merge(gatenext, e2);

					if (realop == CLOSE)
						mesh.merge(gateprev, e1);
					break;
				case CONNBWD:
					mesh.merge(gateprev, e1);
					break;
				}
			}

			prog(vertexIdx);
		}
	} while (1);
	prog.end();
}

}
}
