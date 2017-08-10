#pragma once

#include <unordered_set>
#include <iostream>
#include <cmath>

#include "cutborder.h"
// #include "attrcode.h"
#include "io.h"

#include "../../structs/mesh.h"
#include "../../assert.h"

#include "attrcode.h"


#define INVALID_PAIR mesh::conn::fepair()

struct Perm {
	std::vector<int> perm;

	Perm(int n) : perm(n, -1)
	{}

	bool isMapped(int idx)
	{
		return perm[idx] != -1;
	}

	void map(int a, int b)
	{
		perm[a] = b;
	}
	void unmap(int a)
	{
		perm[a] = -1;
	}
	int get(int idx)
	{
		return perm[idx];
	}
};

struct EncoderData {
	mesh::conn::fepair a;
	int vr;
// 	AttrListElem *elems;

	EncoderData() : a(INVALID_PAIR)
	{}

	void init(mesh::conn::fepair _a, int _vr)
	{
		a = _a;
		vr = _vr;
	}
};
std::ostream &operator<<(std::ostream &os, const EncoderData &v)
{
	return os << v.a;
}

struct CBMStats {
	int used_parts, used_elements;
	int nm;
};

mesh::conn::fepair chooseTriangle(std::unordered_set<mesh::faceidx_t> &remaining_faces)
{
	mesh::faceidx_t f = remaining_faces.find(0) == remaining_faces.end() ? *remaining_faces.begin() : 0;
	mesh::conn::fepair a = mesh::conn::fepair(f, 0);
	remaining_faces.erase(f);
	return a;
}

template <typename C>
mesh::conn::fepair getVertexData(C &conn, std::unordered_set<mesh::faceidx_t> &remaining_faces, mesh::conn::fepair i)
{
	mesh::conn::fepair a = conn.twin(i);
	if (a == i) return INVALID_PAIR;
	if (remaining_faces.find(a.f()) == remaining_faces.end()) return INVALID_PAIR; // this can happen on non manifold edges
	remaining_faces.erase(a.f());
	return a;
}

template <typename H, typename T, typename W, typename P>
CBMStats encode(H &mesh, T &handle, W &wr, P &prog)
{
	attrcode::AttrCoder<W> ac(mesh, wr);

	typedef DataTpl<EncoderData> Data;
	typedef ElementTpl<EncoderData> Element;
	CutBorder<EncoderData> cutBorder(100 + 2000, 10 * std::sqrt(mesh.num_vtx()) + 10000000);
	std::unordered_set<mesh::faceidx_t> remaining_faces;
	for (mesh::faceidx_t i = 0; i < mesh.num_face(); ++i) {
		remaining_faces.insert(i);
	}

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
	std::ofstream osss("order.dbg");
	do {
		Data v0, v1, v2, v2op;
		curtri = 0;
		e0 = chooseTriangle(remaining_faces); e1 = mesh.conn.enext(e0); e2 = mesh.conn.enext(e1);
		v0 = Data(mesh.conn.org(e0)); v1 = Data(mesh.conn.org(e1)); v2 = Data(mesh.conn.org(e2));
		bool m0 = perm.isMapped(v0.idx), m1 = perm.isMapped(v1.idx), m2 = perm.isMapped(v2.idx);
		f = mesh.conn.face(e0);
		ntri = mesh.conn.num_edges(f) - 2;
		assert_eq(ntri, 1);
		CutBorderBase::INITOP initop;
		// Tri 3
		if (m0 && m1 && m2) {
			wr.tri111(ntri, perm.get(v0.idx), perm.get(v1.idx), perm.get(v2.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
// 			ac.vtx(f, e0.e());
			initop = CutBorderBase::TRI111;
			nm += 3;
		}
		// Tri 2
		else if (m0 && m1) {
			wr.tri110(ntri, perm.get(v0.idx), perm.get(v1.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e2.e());
			perm.map(v2.idx, vertexIdx++);
			initop = CutBorderBase::TRI110;
			nm += 2;
		} else if (m1 && m2) {
			wr.tri011(ntri, perm.get(v1.idx), perm.get(v2.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e0.e());
			perm.map(v0.idx, vertexIdx++);
			// swap into decoding order
// 			std::swap(v1, v0); std::swap(e1, e0);
// 			std::swap(v2, v1); std::swap(e2, e1);
			initop = CutBorderBase::TRI011;
			nm += 2;
		} else if (m2 && m0) {
			wr.tri101(ntri, perm.get(v2.idx), perm.get(v0.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e1.e());
			perm.map(v1.idx, vertexIdx++);
			// swap into decoding order
// 			std::swap(v2, v0); std::swap(e2, e0);
// 			std::swap(v2, v1); std::swap(e2, e1);
			initop = CutBorderBase::TRI101;
			nm += 2;
		}
		// Tri 1
		else if (m0) {
			wr.tri100(ntri, perm.get(v0.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e1.e());
			ac.vtx(f, e2.e());
			perm.map(v1.idx, vertexIdx++); perm.map(v2.idx, vertexIdx++);
			initop = CutBorderBase::TRI100;
			++nm;
		} else if (m1) {
			wr.tri010(ntri, perm.get(v1.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e2.e());
			ac.vtx(f, e0.e());
			perm.map(v2.idx, vertexIdx++); perm.map(v0.idx, vertexIdx++);
			// swap into decoding order
// 			std::swap(v1, v0); std::swap(e1, e0);
// 			std::swap(v2, v1); std::swap(e2, e1);
			initop = CutBorderBase::TRI010;
			++nm;
		} else if (m2) {
			wr.tri001(ntri, perm.get(v2.idx));
// 			ac.face(f);
// 			ac.face(f, e0.e());
			ac.vtx(f, e0.e());
			ac.vtx(f, e1.e());
			perm.map(v0.idx, vertexIdx++); perm.map(v1.idx, vertexIdx++);
			// swap into decoding order
// 			std::swap(v2, v0); std::swap(e2, e0);
// 			std::swap(v2, v1); std::swap(e2, e1);
			initop = CutBorderBase::TRI001;
			++nm;
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

		handle(f, curtri, ntri, v0.idx, v1.idx, v2.idx, initop);

		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

// 		ac.wedge(f, 0, v0.idx); ac.wedge(f, 1, v1.idx); ac.wedge(f, 2, v2.idx);

// 		v0.a = e0; v1.a = e1; v2.a = e2;
		Element *cbe0 = cutBorder.initial(v0, v1, v2); // CHECK 3 NEW VERTICES; 3 NEW EDGES
		cutBorder.init(cbe0, e0, v2.idx);
		cutBorder.init(cbe0->next, e1, v0.idx);
		cutBorder.init(cbe0->next->next, ntri == 1 ? e2 : INVALID_PAIR, v1.idx);

		startedge = e0; curedge = e1;
		lastfaceedge = e2;
		++curtri;

		while (!cutBorder.atEnd()) {
			Data ign1, ign2;
			Element *data_gate = cutBorder.traverseStep(ign1, ign2);
			mesh::conn::fepair gate = data_gate->data.a;
// 			bool inner = gate == mesh::Mesh::INVALID_PAIR; TODO
			bool inner = curtri != ntri;
			if (!inner) {
				v0 = Data(mesh.conn.org(gate));
				v1 = Data(mesh.conn.dest(gate));
				lastfaceedge = gate;

				assert_eq(v0.idx, ign1.idx);
				assert_eq(v1.idx, ign2.idx);
// 				v2op = Data(mesh.dest(mesh.enext(gate))); // TODO error here!!!
				v2op = data_gate->data.vr;

				e0 = getVertexData(mesh.conn, remaining_faces, gate);
				startedge = e0;
			} else {
// 				assert_eq(gate, mesh::Mesh::INVALID_PAIR);
				v0 = Data(mesh.conn.dest(curedge));
				v1 = Data(mesh.conn.org(startedge));
				assert_eq(v0.idx, ign1.idx);
				assert_eq(v1.idx, ign2.idx);
// 				v2op = Data(mesh.org(curedge));
				v2op = data_gate->data.vr;
				e0 = INVALID_PAIR;
			}

			int endvtx_order = order[v1.idx];
			osss << endvtx_order << std::endl;
			wr.order(endvtx_order);

			if (!inner && e0 == INVALID_PAIR) {
				handle(f, curtri, ntri, v0.idx, v1.idx, v2op.idx, CutBorderBase::BORDER);
				f = mesh.conn.face(gate);
				CutBorderBase::OP bop = cutBorder.border();
// 				bop = CutBorderBase::BORDER;
				wr.border(bop);
			} else {
				bool lasttri;
				if (!inner) {
					curtri = 0;
					f = mesh.conn.face(e0);
					fop = mesh.conn.face(gate);
					ntri = mesh.conn.num_edges(f) - 2;

					e1 = mesh.conn.enext(e0); e2 = mesh.conn.enext(e1);
					lasttri = mesh.conn.enext(mesh.conn.enext(e1)) == startedge;
					curedge = e1;
					
					assert_eq(mesh.conn.org(e0), v1.idx);
					assert_eq(mesh.conn.dest(e0), v0.idx);
					v2 = Data(mesh.conn.org(e2));
				} else {
					f = mesh.conn.face(startedge); // TODO: not needed
					fop = mesh.conn.face(lastfaceedge);

					e1 = curedge = mesh.conn.enext(curedge);
					lasttri = mesh.conn.enext(mesh.conn.enext(e1)) == startedge;
					if (lasttri) e2 = mesh.conn.enext(e1); // TODO: use ints for if
					else e2 = mesh.conn.enext(curedge); //mesh::Mesh::INVALID_PAIR;

					v2 = Data(mesh.conn.dest(curedge));
				}

				if (!perm.isMapped(v2.idx)) {
					handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::ADDVTX);
					cutBorder.newVertex(v2, e2, v0.idx);
					cutBorder.init(data_gate, e1, v1.idx);
					wr.newvertex(inner ? 0 : ntri); // TODO
// 					if (!inner) ac.face(f);
// 					if (!inner) ac.face(f, e0.e());
					ac.vtx(f, e2.e());
					perm.map(v2.idx, vertexIdx++);
				} else {
					int i, p;
					CutBorderBase::OP op;
					bool succ = cutBorder.findAndUpdate(v2, i, p, op, e2, v0.idx);
					if (!succ) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::NM);
						cutBorder.newVertex(v2, e2, v0.idx);
						cutBorder.init(data_gate, e1, v1.idx);
						wr.nm(inner ? 0 : ntri, perm.get(v2.idx));
// 						if (!inner) ac.face(f, e0.e());
// 						if (!inner) ac.face(f);
						++nm;
// 						e0 = mesh::Mesh::INVALID_PAIR;
					} else if (op == CutBorderBase::UNION) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::UNION);
						wr.cutborderunion(inner ? 0 : ntri, i, p);
// 						if (!inner) ac.face(f, e0.e());
// 						if (!inner) ac.face(f);
						cutBorder.init(data_gate, e1, v1.idx);
					} else if (op == CutBorderBase::CONNFWD) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::CONNFWD);
						wr.connectforward(inner ? 0 : ntri);
// 						if (!inner) ac.face(f, e0.e());
// 						if (!inner) ac.face(f);
						cutBorder.init(data_gate, e1, v1.idx);
					} else if (op == CutBorderBase::CONNBWD) {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::CONNBWD);
						wr.connectbackward(inner ? 0 : ntri);
// 						if (!inner) ac.face(f, e0.e());
// 						if (!inner) ac.face(f);
						cutBorder.init(data_gate, e2, v0.idx);
					} else {
						handle(f, curtri, ntri, v1.idx, v0.idx, v2.idx, CutBorderBase::SPLIT);
						wr.splitcutborder(inner ? 0 : ntri, i);
// 						if (!inner) ac.face(f, e0.e());
// 						if (!inner) ac.face(f);
						cutBorder.init(data_gate, e1, v1.idx);
					}
					assert_eq(curtri + 1 == ntri, lasttri); 
					if (lasttri) cutBorder.preserveOrder();
				}


				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];


				if (!inner) {
// 					ac.wedge(f, 0, v1.idx); ac.wedge(f, 1, v0.idx); ac.wedge(f, 2, v2.idx);
				} else {
// 					ac.wedge(f, curtri + 2, /* mesh.org(f, curtri + 2)*/v2.idx);
				}

				++curtri;
			}

			prog(vertexIdx);
		}
	} while (!remaining_faces.empty());
	wr.end();

	prog.end();

	return CBMStats{ cutBorder.max_parts, cutBorder.max_elements, nm };
}

struct DecoderData {
	mesh::vtxidx_t vr;
	mesh::faceidx_t f;

	void init(int _vr, int _f)
	{
		vr = _vr;
		f = _f;
	}
};
std::ostream &operator<<(std::ostream &os, const DecoderData &v)
{
	return os << v.vr;
}

template <typename H, typename R, typename P>
CBMStats decode(H &builder, R &rd, P &prog)
{
	std::ifstream isss("order.dbg");
	attrcode::AttrDecoder<R> ac(builder, rd);

	typedef DataTpl<DecoderData> Data;
	typedef ElementTpl<DecoderData> Element;

	int nm = 0;

	CutBorder<DecoderData> cutBorder(100 + 2000, 10 * sqrt(builder.num_vtx()) + 10000000);
	std::vector<int> order(builder.num_vtx(), 0);

	int vertexIdx = 0;
	mesh::faceidx_t f = 0;
	int i, p;

	prog.start(builder.num_vtx());

	int curtri, ntri;
// #define ne (ntri + 2)
	do {
		Data v0, v1, v2;
		CutBorderBase::INITOP initop = rd.iop();
		if (initop == CutBorderBase::EOM) break;
		curtri = 0;
		switch (initop) {
		case CutBorderBase::INIT:
			v0.idx = vertexIdx++; v1.idx = vertexIdx++; v2.idx = vertexIdx++;
			ntri = rd.numtri();
// 			ac.face(f, ne);
// 			ac.vtx(/*v0.idx*/f, 0); ac.vtx(/*v1.idx*/f, 1); ac.vtx(/*v2.idx*/f,2);
			break;
		case CutBorderBase::TRI100:
			v0.idx = rd.vertid(); v1.idx = vertexIdx++; v2.idx = vertexIdx++;
			ntri = rd.numtri();
			break;
// 			goto TRI1;
		case CutBorderBase::TRI010:
			v0.idx = vertexIdx++; v1.idx = rd.vertid(); v2.idx = vertexIdx++;
			ntri = rd.numtri();
			break;
// // 			ntri = rd.numtri();
// 			goto TRI1;
		case CutBorderBase::TRI001:
			v0.idx = vertexIdx++; v1.idx = vertexIdx++; v2.idx = rd.vertid();
			ntri = rd.numtri();
			break;
// 		TRI1:
// 			ntri = rd.numtri();
// // 			ac.face(f, ne);
// // 			ac.vtx(/*v1.idx*/f,1); ac.vtx(/*v2.idx*/f,2);
// 			++nm;
// 			break;
		case CutBorderBase::TRI110:
			v0.idx = rd.vertid(); v1.idx = rd.vertid(); v2.idx = vertexIdx++;
// 			goto TRI2;
			ntri = rd.numtri();
			break;
		case CutBorderBase::TRI101:
			v0.idx = rd.vertid(); v1.idx = vertexIdx++; v2.idx = rd.vertid();
// 			goto TRI2;
			ntri = rd.numtri();
			break;
		case CutBorderBase::TRI011:
			v0.idx = vertexIdx++; v1.idx = rd.vertid(); v2.idx = rd.vertid();
			ntri = rd.numtri();
			break;
// 		TRI2:
// 			ntri = rd.numtri();
// // 			ac.face(f, ne);
// // 			ac.vtx(/*v2.idx*/f,2);
// 			nm += 2;
// 			break;
		case CutBorderBase::TRI111:
			v0.idx = rd.vertid(); v1.idx = rd.vertid(); v2.idx = rd.vertid();
// 			ntri = rd.numtri();
// 			std::cout << ntri << std::endl;
// 			ac.face(f, ne);
// 			nm += 3;
// 			break;
			ntri = rd.numtri();
			break;
		}
		std::cout << "sf1? " << v0.idx << " " << v1.idx << " " << v2.idx << std::endl;
		++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];
		std::cout << "sf1.1?" << std::endl;

// 		ac.wedge(f, 0, v0.idx); ac.wedge(f, 1, v1.idx); ac.wedge(f, 2, v2.idx);

// 		handle.face(v0.idx, v1.idx, v2.idx, CutBorderBase::NM); // no init
		builder.face_begin(ntri + 2); builder.set_org(v0.idx); builder.set_org(v1.idx); builder.set_org(v2.idx);
		std::cout << "sf1.2?" << std::endl;

		++curtri;
		if (curtri == ntri) {
			builder.face_end();
		}

		switch (initop) {
		case CutBorderBase::INIT:
			ac.vtx(/*v0.idx*/f, 0); ac.vtx(/*v1.idx*/f, 1); ac.vtx(/*v2.idx*/f,2);
			break;
		case CutBorderBase::TRI100:
			ac.vtx(/*v1.idx*/f,1); ac.vtx(/*v2.idx*/f,2);
			break;
		case CutBorderBase::TRI010:
			ac.vtx(/*v1.idx*/f,0); ac.vtx(/*v2.idx*/f,2);
			break;
		case CutBorderBase::TRI001:
			ac.vtx(/*v1.idx*/f,0); ac.vtx(/*v2.idx*/f,1);
			break;
		case CutBorderBase::TRI110:
			ac.vtx(/*v2.idx*/f,2);
			break;
		case CutBorderBase::TRI101:
			ac.vtx(/*v2.idx*/f,1);
			break;
		case CutBorderBase::TRI011:
			ac.vtx(/*v2.idx*/f,0);
			break;
		case CutBorderBase::TRI111:
			break;
		}
		std::cout << "sf2?" << std::endl;

		v0.vr = v2.idx; v1.vr = v0.idx; v2.vr = v1.idx; // TODO
		v0.f = f; v1.f = f; v2.f = f;
		cutBorder.initial(v0, v1, v2);

		if (curtri == ntri) {
			++f;
// 			assert_eq(builder.num_face(), f);
		}
		std::cout << "sf3?" << std::endl;

		while (!cutBorder.atEnd()) {
			Element *data_gate = cutBorder.traverseStep(v0, v1);
			mesh::vtxidx_t gatevr = data_gate->data.vr;
			mesh::faceidx_t gateface = data_gate->data.f;

			int endvtx_order = order[v1.idx];
			rd.order(endvtx_order);

			int ord2; isss >> ord2;
			if (endvtx_order != ord2) {
// 				std::cout << "ERROR: order " << endvtx_order << " exp: " << ord2 << std::endl; std::exit(1);
			}

			CutBorderBase::OP op = rd.op();
// 			std::cout << "op: " << CutBorderBase::op2str(op) << std::endl;

			bool inner = curtri != ntri;

			switch (op) {
			case CutBorderBase::CONNFWD:
				v2 = cutBorder.connectForward();
				if (!v2.isUndefined()) {
					if (!inner) { ntri = rd.numtri(); curtri = 0; builder.face_begin(ntri + 2); builder.set_org(v1.idx); builder.set_org(v0.idx); builder.set_org(v2.idx); /*ac.face(f, gateface, ne);*/ }
					else builder.set_org(v2.idx);
// 					handle.face(v1.idx, v0.idx, v2.idx, op);
					cutBorder.init(data_gate, v1.idx, f);
				} else {
// 					handle(v1.v, v0.v, op);
				}
				break;
			case CutBorderBase::CONNBWD:
				v2 = cutBorder.connectBackward();
				if (!v2.isUndefined()) {
					if (!inner) { ntri = rd.numtri(); curtri = 0; builder.face_begin(ntri + 2); builder.set_org(v1.idx); builder.set_org(v0.idx); builder.set_org(v2.idx);/* ac.face(f, gateface, ne);*/ }
					else builder.set_org(v2.idx);
// 					handle.face(v1.idx, v0.idx, v2.idx, op);
					cutBorder.init(data_gate, v0.idx, f);
				} else {
// 					handle(v1.v, v0.v, op);
				}
				break;
			case CutBorderBase::SPLIT:
				i = rd.elem();
				v2 = cutBorder.splitCutBorder(i, v0.idx, f);
				if (!inner) { ntri = rd.numtri(); curtri = 0; builder.face_begin(ntri + 2); builder.set_org(v1.idx); builder.set_org(v0.idx); builder.set_org(v2.idx); /*ac.face(f, gateface, ne);*/ }
				else builder.set_org(v2.idx);
// 				handle.face(v1.idx, v0.idx, v2.idx, op);
				cutBorder.init(data_gate, v1.idx, f);
				break;
			case CutBorderBase::UNION:
				i = rd.elem();
				p = rd.part();
				v2 = cutBorder.cutBorderUnion(i, p, v0.idx, f);
				if (!inner) { ntri = rd.numtri(); curtri = 0; builder.face_begin(ntri + 2); builder.set_org(v1.idx); builder.set_org(v0.idx); builder.set_org(v2.idx); /*ac.face(f, gateface, ne);*/ }
				else builder.set_org(v2.idx);
// 				handle.face(v1.idx, v0.idx, v2.idx, op);
				cutBorder.init(data_gate, v1.idx, f);
				break;
			case CutBorderBase::ADDVTX:
				v2 = Data(vertexIdx++);
				if (!inner) { ntri = rd.numtri(); curtri = 0; builder.face_begin(ntri + 2); builder.set_org(v1.idx); builder.set_org(v0.idx); builder.set_org(v2.idx); /*ac.face(f, gateface, ne);*/ }
				else builder.set_org(v2.idx);
// 				ac.vtx(v2.idx, v0.idx, v1.idx, gatevr);
				ac.vtx(f, curtri + 2);
				cutBorder.newVertex(v2, v0.idx, f);
				cutBorder.init(data_gate, v1.idx, f);
// 				handle.face(v1.idx, v0.idx, v2.idx, op);
				break;
			case CutBorderBase::NM:
				v2.idx = rd.vertid();
				if (!inner) { ntri = rd.numtri(); curtri = 0; builder.face_begin(ntri + 2); builder.set_org(v1.idx); builder.set_org(v0.idx); builder.set_org(v2.idx); /*ac.face(f, gateface, ne);*/ }
				else builder.set_org(v2.idx);
// 				handle.face(v1.idx, v0.idx, v2.idx, CutBorderBase::NM);

				cutBorder.newVertex(v2, v0.idx, f);
				cutBorder.init(data_gate, v1.idx, f);
// 				v2 = Data(-1);
				++nm;
				break;
			case CutBorderBase::BORDER:
				cutBorder.border();
// 				handle(v1.v, v0.v, op);
				v2 = Data(-1);
				break;
			}

			if (!v2.isUndefined()) {
				++order[v0.idx]; ++order[v1.idx]; ++order[v2.idx];

// 				if (!inner) {
// 					ac.wedge(f, 0, v1.idx); ac.wedge(f, 1, v0.idx); ac.wedge(f, 2, v2.idx);
// 				} else {
// 					ac.wedge(f, curtri + 2, v2.idx);
// 				}
				++curtri;
				if (curtri == ntri) {
					builder.face_end();
// 					builder.end_face();
					cutBorder.preserveOrder();
					++f;
// 					assert_eq(builder.num_face(), f);
				}
			}

			prog(vertexIdx);
		}
		std::cout << "sf4?" << std::endl;
	} while (1);
	prog.end();

	ac.decode();

	return CBMStats{ cutBorder.max_parts, cutBorder.max_elements, nm };
}


//358780bytes for bunny
//297664bytes after diff
//24063bytes
//298018bytes
//297880bytes
//271340
//271354bytes
//261379bytes
//261363bytes
//261364
//261375
//409383 with attr

// globe:
//1915068
//1915024 // NM OP introduced
//1913730 // removed 2 redudant vertids
//1913734
//1912111 // nm changed border to newvertex
//1912116
//1911926 // sperated to init-symbol context
//1911914
//1890733 // fixed symbol model
//4260149 with attributes
//4065502
