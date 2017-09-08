#pragma once

#include <vector>
#include <cmath>
#include <iostream>

#include "cutborder.h"
#include "cbm_base.h"
#include "io.h"

#include "../../assert.h"

#include "attrcode.h"

template <typename R, typename P>
CBMStats decode(mesh::Builder &builder, R &rd, attrcode::AttrDecoder<R> &ac, P &prog)
{
	builder.noautomerge();

	typedef DataTpl<CoderData> Data;
	typedef ElementTpl<CoderData> Element;

	int nm = 0;

	CutBorder<CoderData> cutBorder(100 + 20000, 10 * sqrt(builder.num_vtx()) + 10000000);
	std::vector<int> order(builder.num_vtx(), 0);

	int vertexIdx = 0;
	mesh::faceidx_t f = 0;
	int i, p;

	prog.start(builder.num_vtx());

	int curtri, ntri;
	mesh::conn::fepair startedge, curedge, lastfaceedge;
	do {
		Data v0, v1, v2;
		CutBorderBase::INITOP initop = rd.iop();
		if (initop == CutBorderBase::EOM) break;
		curtri = 0;
		switch (initop) {
		case CutBorderBase::INIT:
			v0.idx = vertexIdx++; v1.idx = vertexIdx++; v2.idx = vertexIdx++;
			break;
		case CutBorderBase::TRI100:
			v0.idx = rd.vertid(); v1.idx = vertexIdx++; v2.idx = vertexIdx++;
			break;
		case CutBorderBase::TRI010:
			v2.idx = vertexIdx++; v1.idx = rd.vertid(); v0.idx = vertexIdx++;
			break;
		case CutBorderBase::TRI001:
			v0.idx = vertexIdx++; v1.idx = vertexIdx++; v2.idx = rd.vertid();
			break;
		case CutBorderBase::TRI110:
			v0.idx = rd.vertid(); v1.idx = rd.vertid(); v2.idx = vertexIdx++;
			break;
		case CutBorderBase::TRI101:
			v2.idx = rd.vertid(); v1.idx = vertexIdx++; v0.idx = rd.vertid();
			break;
		case CutBorderBase::TRI011:
			v0.idx = vertexIdx++; v1.idx = rd.vertid(); v2.idx = rd.vertid();
			break;
		case CutBorderBase::TRI111:
			v0.idx = rd.vertid(); v1.idx = rd.vertid(); v2.idx = rd.vertid();
			break;
		}
		ntri = rd.numtri();
		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

		builder.face_begin(ntri + 2); builder.set_org(v0.idx); builder.set_org(v1.idx); builder.set_org(v2.idx);

		++curtri;
		if (curtri == ntri) builder.face_end();

		switch (initop) {
		case CutBorderBase::INIT:
			ac.vtx(f, 0); ac.vtx(f, 1); ac.vtx(f, 2);
			break;
		case CutBorderBase::TRI100:
			ac.vtx(f, 1); ac.vtx(f, 2);
			break;
		case CutBorderBase::TRI010:
			ac.vtx(f, 2); ac.vtx(f, 0);
			break;
		case CutBorderBase::TRI001:
			ac.vtx(f, 0); ac.vtx(f, 1);
			break;
		case CutBorderBase::TRI110:
			ac.vtx(f, 2);
			break;
		case CutBorderBase::TRI101:
			ac.vtx(f, 1);
			break;
		case CutBorderBase::TRI011:
			ac.vtx(f, 0);
			break;
		case CutBorderBase::TRI111:
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
			Element *elm_gate = cutBorder.traverseStep(v0, v1);
			Data *data_gate = &elm_gate->data;
			mesh::conn::fepair gateedge = data_gate->a;
			mesh::conn::fepair gateedgeprev = elm_gate->prev->data.a;
			mesh::conn::fepair gateedgenext = elm_gate->next->data.a;
// 			mesh::faceidx_t gateface = /*gateedge.f()*/data_gate->f;

			rd.order(order[v1.idx]);

			CutBorderBase::OP op = rd.op(), realop;

			bool seq_first = curtri == ntri;

			switch (op) {
			case CutBorderBase::CONNFWD:
				v2 = cutBorder.connectForward(realop); // TODO: add border to realop
				break;
			case CutBorderBase::CONNBWD:
				v2 = cutBorder.connectBackward(realop); // TODO: add border to realop
				break;
			case CutBorderBase::SPLIT:
				i = rd.elem();
				v2 = cutBorder.splitCutBorder(i);
				break;
			case CutBorderBase::UNION:
				i = rd.elem();
				p = rd.part();
				v2 = cutBorder.cutBorderUnion(i, p);
				break;
			case CutBorderBase::ADDVTX:
				v2 = Data(vertexIdx++);
				cutBorder.newVertex(v2);
				break;
			case CutBorderBase::NM:
				v2.idx = rd.vertid();
				cutBorder.newVertex(v2);
				++nm;
				break;
			case CutBorderBase::BORDER:
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
				case CutBorderBase::CONNFWD:
					data_gate->init(e1);
					break;
				case CutBorderBase::CONNBWD:
					data_gate->init(e2);
					break;
				case CutBorderBase::SPLIT:
				case CutBorderBase::UNION:
				case CutBorderBase::ADDVTX:
				case CutBorderBase::NM:
					cutBorder.last->init(e2);
					data_gate->init(e1);
					break;
				}

				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

// 				if (seq_first) {
// 					ac.wedge(f, 0, v1.idx); ac.wedge(f, 1, v0.idx); ac.wedge(f, 2, v2.idx);
// 				} else {
// 					ac.wedge(f, curtri + 2, v2.idx);
// 				}
				if (op == CutBorderBase::ADDVTX) ac.vtx(f, curtri + 2);
				++curtri;
				if (seq_last) {
					builder.face_end();
					cutBorder.preserveOrder();
					++f;
				}

				if (seq_first) {
					assert_eq(builder.mesh.conn.twin(gateedge), gateedge);
					assert_eq(builder.mesh.conn.twin(e0), e0);
					builder.mesh.conn.fmerge(gateedge, e0);
				}

				switch (op) {
				case CutBorderBase::CONNFWD:
					if (seq_last) {
						assert_eq(builder.mesh.conn.twin(gateedgenext), gateedgenext);
						assert_eq(builder.mesh.conn.twin(e2), e2);
						builder.mesh.conn.fmerge(gateedgenext, e2);
					}

					if (realop == CutBorderBase::CLOSEFWD) {
						assert_eq(builder.mesh.conn.twin(gateedgeprev), gateedgeprev);
						assert_eq(builder.mesh.conn.twin(e1), e1);
						builder.mesh.conn.fmerge(gateedgeprev, e1);
					}
					break;
				case CutBorderBase::CONNBWD:
					assert_eq(builder.mesh.conn.twin(gateedgeprev), gateedgeprev);
					assert_eq(builder.mesh.conn.twin(e1), e1);
					builder.mesh.conn.fmerge(gateedgeprev, e1);

					if (seq_last && realop == CutBorderBase::CLOSEBWD) {
						assert_eq(builder.mesh.conn.twin(gateedgenext), gateedgenext);
						assert_eq(builder.mesh.conn.twin(e2), e2);
						builder.mesh.conn.fmerge(gateedgenext, e2);
					}
					break;
				}
			}

			prog(vertexIdx);
		}
	} while (1);
	prog.end();

	return CBMStats{ cutBorder.max_parts, cutBorder.max_elements, nm };
}
