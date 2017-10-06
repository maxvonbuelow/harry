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
#include <cmath>
#include <iostream>

#include "cutborder.h"
#include "cbm_base.h"
#include "io.h"

#include "../../assert.h"

#include "attrcode.h"

namespace hry {
namespace cbm {

template <typename R, typename P>
void decode(mesh::Builder &builder, R &rd, attrcode::AttrDecoder<R> &ac, P &prog)
{
	builder.noautomerge();

	typedef DataTpl<CoderData> Data;
	typedef ElementTpl<CoderData> Element;

	int nm = 0;

	CutBorder<CoderData> cutBorder(builder.num_vtx());
	std::vector<int> order(builder.num_vtx(), 0);

	int vertexIdx = 0;
	mesh::faceidx_t f = 0;
	int i, p;

	prog.start(builder.num_vtx());

	int curtri, ntri;
	mesh::conn::fepair startedge, curedge, lastfaceedge;
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

		builder.face_begin(ntri + 2); builder.set_org(v0.idx); builder.set_org(v1.idx); builder.set_org(v2.idx);

		++curtri;
		if (curtri == ntri) builder.face_end();

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

		mesh::conn::fepair e0(f, 0), e1(f, 1), e2(f, 2);
		v0.init(e0);
		v1.init(e1);
		v2.init(e2);

		assert_eq(builder.mesh.conn.org(e0), v0.idx);
		assert_eq(builder.mesh.conn.org(e1), v1.idx);
		assert_eq(builder.mesh.conn.org(e2), v2.idx);
		cutBorder.initial(v0, v1, v2);

		if (curtri == ntri) ++f;

		startedge = e0; curedge = e1;
		lastfaceedge = e2;

		while (!cutBorder.atEnd()) {
			cutBorder.traverseStep(v0, v1);
			mesh::conn::fepair gateedge = v0.a;
			mesh::conn::fepair gateedgeprev = cutBorder.left().a;
			mesh::conn::fepair gateedgenext = cutBorder.right().a;

			rd.order(order[v1.idx]);

			OP op = rd.op(), realop;

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
				++nm;
				break;
			case BORDER:
				cutBorder.border();
				v2 = Data(-1);
				assert_eq(builder.mesh.conn.twin(gateedge), gateedge);
				break;
			}

			if (!v2.isUndefined()) {
				if (seq_first) {
					ntri = rd.numtri();
					curtri = 0;
					builder.face_begin(ntri + 2);
					builder.set_org(v1.idx);
					builder.set_org(v0.idx);
					builder.set_org(v2.idx);
					e0 = mesh::conn::fepair(f, 0); e1 = curedge = mesh::conn::fepair(f, 1); e2 = mesh::conn::fepair(f, 2);
				} else {
					builder.set_org(v2.idx);
					e0 = INVALID_PAIR;
					e1 = curedge = builder.mesh.conn.enext(curedge);
					e2 = builder.mesh.conn.enext(e1);
				}
				bool seq_last = curtri + 1 == ntri;
				bool seq_mid = !seq_first && !seq_last;
				assert_eq(builder.builder_conn.cur_f, f);

				switch (op) {
				case CONNFWD:
					if (realop != CLOSE) cutBorder.first->init(e1);
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
				if (seq_last) {
					builder.face_end();
					++f;
				}

				if (seq_first) {
					assert_eq(builder.mesh.conn.twin(gateedge), gateedge);
					assert_eq(builder.mesh.conn.twin(e0), e0);
					builder.mesh.conn.fmerge(gateedge, e0);
				}

				switch (op) {
				case CONNFWD:
					if (seq_last) {
						assert_eq(builder.mesh.conn.twin(gateedgenext), gateedgenext);
						assert_eq(builder.mesh.conn.twin(e2), e2);
						builder.mesh.conn.fmerge(gateedgenext, e2);
					}

					if (realop == CLOSE) {
						assert_eq(builder.mesh.conn.twin(gateedgeprev), gateedgeprev);
						assert_eq(builder.mesh.conn.twin(e1), e1);
						builder.mesh.conn.fmerge(gateedgeprev, e1);
					}
					break;
				case CONNBWD:
					assert_eq(builder.mesh.conn.twin(gateedgeprev), gateedgeprev);
					assert_eq(builder.mesh.conn.twin(e1), e1);
					builder.mesh.conn.fmerge(gateedgeprev, e1);

// 					if (seq_last && realop == CLOSEBWD) {
// 						assert_eq(builder.mesh.conn.twin(gateedgenext), gateedgenext);
// 						assert_eq(builder.mesh.conn.twin(e2), e2);
// 						builder.mesh.conn.fmerge(gateedgenext, e2);
// 					}
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
