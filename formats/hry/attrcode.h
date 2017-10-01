/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include "io.h"
#include "transform.h"
#include "prediction.h"

namespace attrcode {

static const mesh::attridx_t UNSET = std::numeric_limits<mesh::attridx_t>::max();
struct GlobalHistory {
	std::vector<mesh::attridx_t> tidxlist;
	mesh::attridx_t tidx;

	GlobalHistory() : tidx(0)
	{}

	void resize(mesh::attridx_t size)
	{
		tidxlist.resize(size, UNSET);
	}

	void set(mesh::attridx_t idx)
	{
		tidxlist[idx] = tidx++;
	}
	mesh::attridx_t gget(mesh::attridx_t idx)
	{
		return tidxlist[idx];
	}
	mesh::attridx_t lget_set(mesh::attridx_t idx)
	{
		mesh::attridx_t g = gget(idx);
		if (g == UNSET) {
			set(idx);
			return UNSET;
		} else {
			return tidx - 1 - g;
		}
	}
};
struct LocalHistory {
	std::vector<std::vector<mesh::attridx_t>> hist; // TODO: this results in a vector<vector<vector<X>>>, reduce this to max 2 containers

	void resize(mesh::vtxidx_t size)
	{
		hist.resize(size);
	}

	bool empty(mesh::vtxidx_t v)
	{
		return hist[v].empty();
	}

	mesh::attridx_t insert(mesh::vtxidx_t v, mesh::attridx_t idx)
	{
		for (mesh::attridx_t i = 0; i < hist[v].size(); ++i) {
			if (hist[v][i] == idx) return hist[v].size() - 1 - i;
		}
		hist[v].push_back(idx);
		return UNSET;
	}

	mesh::attridx_t find(mesh::vtxidx_t v, mesh::attridx_t off)
	{
		return hist[v][hist[v].size() - 1 - off];
	}
};

// This is why my collegues hate me:
#define TFAN_IT(CB) \
do { \
	mesh::conn::fepair e = ein, t; \
	do { \
		CB(e, r); \
		t = mesh.conn.twin(e); \
		if (t == e) goto BWD; \
		e = mesh.conn.enext(t); \
	} while (e != ein); \
	return; \
\
BWD: \
	e = mesh.conn.eprev(ein); \
	t = mesh.conn.twin(e); \
	if (e == t) return; \
	e = t; \
	do { \
		CB(e, r); \
		e = mesh.conn.eprev(e); \
		t = mesh.conn.twin(e); \
		if (e == t) break; \
		e = t; \
	} while (e != ein); \
} while (0)

struct AbsAttrCoder {
	std::vector<bool> vtx_is_encoded;
	std::vector<bool> face_is_encoded;
	int curparal, curneigh, curhist;
	mesh::Mesh &mesh;

	AbsAttrCoder(mesh::Mesh &_mesh) : mesh(_mesh), vtx_is_encoded(_mesh.attrs.num_vtx(), false), face_is_encoded(_mesh.attrs.num_face(), false)
	{}

	void use_paral(mesh::vtxidx_t v0, mesh::vtxidx_t v1, mesh::vtxidx_t vo, mesh::regidx_t r)
	{
		if (!vtx_is_encoded[v0] || !vtx_is_encoded[v1] || !vtx_is_encoded[vo]) return;
		mesh::regidx_t r0 = mesh.attrs.vtx2reg(v0), r1 = mesh.attrs.vtx2reg(v1), ro = mesh.attrs.vtx2reg(vo);
		if (r0 != r || r1 != r || ro != r) return;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_vtxlist(r, a);

			// fetch values
			mixing::View d0 = mesh.attrs[l][mesh.attrs.binding_vtx_attr(v0, a)], d1 = mesh.attrs[l][mesh.attrs.binding_vtx_attr(v1, a)], dop = mesh.attrs[l][mesh.attrs.binding_vtx_attr(vo, a)];

			// add prediction
			if (mesh.attrs[l].cache().size() <= curparal) mesh.attrs[l].cache().resize(curparal + 1);
			mesh.attrs[l].cache()[curparal].setq([] (int q, const auto d0c, const auto d1c, const auto doc) { return pred::predict(d0c, d1c, doc, q); }, d0, d1, dop);
		}
		++curparal;
	}
	void use_corner(mesh::conn::fepair e, mesh::regidx_t r)
	{
		mesh::faceidx_t f = e.f();
		mesh::ledgeidx_t lv = e.e();
		if (!face_is_encoded[f]) return;
		mesh::regidx_t r0 = mesh.attrs.face2reg(f);
		if (r0 != r) return;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_corner_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_cornerlist(r, a);

			// fetch values
			mixing::View d0 = mesh.attrs[l][mesh.attrs.binding_corner_attr(f, lv, a)];

			// add prediction
			if (mesh.attrs[l].cache().size() <= curhist) mesh.attrs[l].cache().resize(curhist + 1);
			mesh.attrs[l].cache()[curhist].setq([] (int q, const auto d0c) { return pred::predict_face(d0c, q); }, d0);
		}
		++curhist;
	}
	void paral(mesh::conn::fepair ein, mesh::regidx_t r)
	{
		mesh::conn::fepair e = ein, e0, e1, t;
		if (mesh.conn.num_edges(e.f()) == 3) {
			e = mesh.conn.enext(e);
			t = mesh.conn.twin(e);
			if (t == e) return;
			e = mesh.conn.enext(mesh.conn.enext(t));
			use_paral(mesh.conn.org(t), mesh.conn.dest(t), mesh.conn.org(e), r);
			return;
		}
		e0 = mesh.conn.enext(e);
		e1 = mesh.conn.eprev(e);
		use_paral(mesh.conn.org(e0), mesh.conn.org(e1), mesh.conn.dest(e0), r);
		if (mesh.conn.num_edges(e.f()) > 4) // when the polygon is a pentagon or more, we have two parallelograms
			use_paral(mesh.conn.org(e0), mesh.conn.org(e1), mesh.conn.org(mesh.conn.eprev(e)), r);
	}

	void tfan(mesh::conn::fepair ein, mesh::regidx_t r)
	{
		TFAN_IT(paral);
	}
	void tfan_corner(mesh::conn::fepair ein, mesh::regidx_t r)
	{
		TFAN_IT(use_corner);
	}

	void get_prediction(mesh::listidx_t l, int num_parts)
	{
		// compute average
		mixing::View avg = mesh.attrs[l].big()[0];
		avg.set([] (const auto) { return 0; }, avg);
		for (int i = 0; i < num_parts; ++i) {
			avg.sets([] (const auto cur, const auto val) { return cur + val; }, avg, mesh.attrs[l].cache()[i]);
		}
		avg.set([num_parts] (const auto cur) { return num_parts == 0 ? decltype(cur)(0) : transform::divround(cur, (decltype(cur))num_parts); }, avg);

		// selection (without quantization or integral values: average; with quantization: value closest to quantization)
		mixing::View res = mesh.attrs[l].accu()[0];
		if (num_parts == 0) {
			res.set([] (const auto x) { return decltype(x)(0); }, res);
		} else {
			res.set([] (const auto x) { return std::numeric_limits<decltype(x)>::max(); }, res);
			for (int i = 0; i < num_parts; ++i) {
				res.setst([] (mixing::Type t, const auto resv, const auto predv, const auto avgv) {
					if (t != mixing::DOUBLE && t != mixing::FLOAT) return avgv;

					decltype(resv) resdiff = avgv > resv ? avgv - resv : resv - avgv;
					decltype(predv) preddiff = avgv > predv ? avgv - predv : predv - avgv;
					return resdiff < preddiff ? resv : predv;
				}, res, mesh.attrs[l].cache()[i], avg);
			}
		}
	}
	void vtx(mesh::faceidx_t ff, mesh::ledgeidx_t ee)
	{
		// TODO: ULONG and LONG must handled seperately: div first add then
		mesh::conn::fepair e(ff, ee);
		mesh::vtxidx_t v = mesh.conn.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);

		curparal = 0;
		tfan(e, r);
		vtx_is_encoded[v] = true;
		int num_paral = curparal;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_vtxlist(r, a);
			get_prediction(l, num_paral);
		}
	}

	void use_neigh(mesh::faceidx_t f, mesh::regidx_t r)
	{
		if (!face_is_encoded[f]) return; // TODO: ordered faces when decoding
		mesh::regidx_t r0 = mesh.attrs.face2reg(f);
		if (r0 != r) return;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_face_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_facelist(r, a);

			// fetch values
			mixing::View d0 = mesh.attrs[l][mesh.attrs.binding_face_attr(f, a)];

			// add prediction
			if (mesh.attrs[l].cache().size() <= curneigh) mesh.attrs[l].cache().resize(curneigh + 1);
			mesh.attrs[l].cache()[curneigh].setq([] (int q, const auto d0c) { return pred::predict_face(d0c, q); }, d0);
		}
		++curneigh;
	}
	void neighs(mesh::conn::fepair e, mesh::regidx_t r)
	{
		mesh::conn::fepair cur = e;
		do {
			// use cur
			mesh::conn::fepair n = mesh.conn.twin(cur);
			if (n != cur) use_neigh(cur.f(), r);
			cur = mesh.conn.enext(cur);
		} while (cur != e);
	}
	void face(mesh::faceidx_t f, mesh::ledgeidx_t ee)
	{
		mesh::regidx_t r = mesh.attrs.face2reg(f);
		mesh::conn::fepair e(f, ee);

		// get neighs
		curneigh = 0;
		neighs(e, r);
		face_is_encoded[f] = true;
		int num_neigh = curneigh;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_face_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_facelist(r, a);
			get_prediction(l, num_neigh);
		}
	}

	void corner(mesh::faceidx_t f, mesh::ledgeidx_t ee) // WARNING: needs to be called AFTER faces
	{
		mesh::regidx_t r = mesh.attrs.face2reg(f);
		mesh::conn::fepair e(f, ee);

		// get hist
		curhist = 0;
		assert(face_is_encoded[f]);
		face_is_encoded[f] = false;
		tfan_corner(e, r);
		face_is_encoded[f] = true;
		int num_hist = curhist;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_corner_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_cornerlist(r, a);
			get_prediction(l, num_hist);
		}
	}
};

template <typename WR>
struct AttrCoder : AbsAttrCoder {
	mesh::Mesh &mesh;
	WR &wr;
	std::vector<GlobalHistory> ghist;
	std::vector<LocalHistory> lhist;
	std::vector<mesh::conn::fepair> order;
	std::vector<mesh::conn::fepair> order_f;

	AttrCoder(mesh::Mesh &_mesh, WR &_wr) : mesh(_mesh), wr(_wr), AbsAttrCoder(_mesh), ghist(_mesh.attrs.size()), lhist(mesh.attrs.num_bindings_corner)
	{
		for (mesh::listidx_t i = 0; i < mesh.attrs.size(); ++i) {
			ghist[i].resize(mesh.attrs[i].size());
		}
		for (mesh::listidx_t i = 0; i < mesh.attrs.num_bindings_corner; ++i) {
			lhist[i].resize(mesh.attrs.num_vtx());
		}
	}

	void vtx(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		order.push_back(e);
	}
	void face(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		order_f.push_back(e);
	}

	void vtx_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		mesh::vtxidx_t v = mesh.conn.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);

		AbsAttrCoder::vtx(f, le);
		wr.reg_vtx(r);

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_vtxlist(r, a);
			mesh::attridx_t idx = mesh.attrs.binding_vtx_attr(v, a);

			mesh::attridx_t tidx = ghist[l].lget_set(idx);
			if (tidx != UNSET) {
				wr.attr_ghist(tidx, l);
				continue;
			}

			mixing::View res = mesh.attrs[l].accu()[0];
			res.setq([] (int q, const auto raw, const auto pred) { return pred::encodeDelta(raw, pred, q); }, mesh.attrs[l][idx], res);
			wr.attr_data(res, l);
		}
	}
	void face_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::regidx_t r = mesh.attrs.face2reg(f);

		AbsAttrCoder::face(f, le);
		wr.reg_face(r);

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_face_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_facelist(r, a);
			mesh::attridx_t idx = mesh.attrs.binding_face_attr(f, a);

			mesh::attridx_t tidx = ghist[l].lget_set(idx);
			if (tidx != UNSET) {
				wr.attr_ghist(tidx, l);
				continue;
			}

			mixing::View res = mesh.attrs[l].accu()[0];
			res.setq([] (int q, const auto raw, const auto pred) { return pred::encodeDelta(raw, pred, q); }, mesh.attrs[l][idx], res);
			wr.attr_data(res, l);
		}
	}
	void corner_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::regidx_t r = mesh.attrs.face2reg(f);

		AbsAttrCoder::corner(f, le);

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_corner_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_cornerlist(r, a);
			mesh::attridx_t idx = mesh.attrs.binding_corner_attr(f, le, a);

			mesh::attridx_t lidx = lhist[a].insert(mesh.conn.org(f, le), idx);
			if (lidx != UNSET) {
				wr.attr_lhist(lidx, l);
				continue;
			}

			mesh::attridx_t tidx = ghist[l].lget_set(idx);
			if (tidx != UNSET) {
				wr.attr_ghist(tidx, l);
				continue;
			}

			mixing::View res = mesh.attrs[l].accu()[0];
			res.setq([] (int q, const auto raw, const auto pred) { return pred::encodeDelta(raw, pred, q); }, mesh.attrs[l][idx], res);
			wr.attr_data(res, l);
		}
	}

	template <typename P>
	void encode(P &prog)
	{
		prog.start(order.size());
		for (int i = 0; i < order.size(); ++i) {
			mesh::conn::fepair &e = order[i];

			vtx_post(e.f(), e.e());
			prog(i);
		}
		for (int i = 0; i < order_f.size(); ++i) {
			mesh::conn::fepair &e = order_f[i];
			face_post(e.f(), e.e());
			int ne = mesh.conn.num_edges(e.f()), c = e.e();
			do {
				corner_post(e.f(), c);
				++c;
				if (c == ne) c = 0;
			} while (c != e.e());
		}
		prog.end();
	}
};


template <typename RD>
struct AttrDecoder : AbsAttrCoder {
	RD &rd;

	std::vector<LocalHistory> lhist;
	std::vector<mesh::attridx_t> cur_idx;

	mesh::Builder &builder;
	std::vector<mesh::conn::fepair> order;
	
	AttrDecoder(mesh::Builder &_builder, RD &_rd) : builder(_builder), rd(_rd), AbsAttrCoder(_builder.mesh), lhist(mesh.attrs.num_bindings_corner), cur_idx(_builder.mesh.attrs.size(), 0)
	{
		for (mesh::listidx_t i = 0; i < mesh.attrs.num_bindings_corner; ++i) {
			lhist[i].resize(mesh.attrs.num_vtx());
		}
	}

	void vtx(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		order.push_back(e);
	}

	void vtx_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		mesh::vtxidx_t v = builder.mesh.conn.org(e);
		mesh::regidx_t r = rd.reg_vtx();
		builder.vtx_reg(v, r);

		AbsAttrCoder::vtx(f, le);

		for (mesh::listidx_t a = 0; a < builder.mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = builder.mesh.attrs.binding_reg_vtxlist(r, a);
			mesh::attridx_t idx;

			switch (rd.attr_type(l)) {
			case DATA:
				idx = cur_idx[l]++;
				rd.attr_data(builder.mesh.attrs[l][idx], l);

				mesh.attrs[l][idx].setq([] (int q, const auto delta, const auto pred) { return pred::decodeDelta(delta, pred, q); }, mesh.attrs[l][idx], mesh.attrs[l].accu()[0]);
				break;
			case HIST:
				idx = cur_idx[l] - 1 - rd.attr_ghist(l);
				break;
			}

			builder.bind_vtx_attr(v, a, idx);
		}
	}

	void face(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		// order is obvious: [ 0 0 ], [ 1 0 ], [ 2 0 ]...
	}
	void face_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::regidx_t r = rd.reg_face();
		builder.face_reg(f, r);

		AbsAttrCoder::face(f, le);

		for (mesh::listidx_t a = 0; a < builder.mesh.attrs.num_bindings_face_reg(r); ++a) {
			mesh::listidx_t l = builder.mesh.attrs.binding_reg_facelist(r, a);
			mesh::attridx_t idx;

			switch (rd.attr_type(l)) {
			case DATA:
				idx = cur_idx[l]++;
				rd.attr_data(builder.mesh.attrs[l][idx], l);

				mesh.attrs[l][idx].setq([] (int q, const auto delta, const auto pred) { return pred::decodeDelta(delta, pred, q); }, mesh.attrs[l][idx], mesh.attrs[l].accu()[0]);
				break;
			case HIST:
				idx = cur_idx[l] - 1 - rd.attr_ghist(l);
				break;
			}

			builder.bind_face_attr(f, a, idx);
		}
	}
	void corner_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::regidx_t r = mesh.attrs.face2reg(f); // face region has already been read before

		AbsAttrCoder::corner(f, le);

		for (mesh::listidx_t a = 0; a < builder.mesh.attrs.num_bindings_corner_reg(r); ++a) {
			mesh::listidx_t l = builder.mesh.attrs.binding_reg_cornerlist(r, a);
			mesh::attridx_t idx;

			switch (rd.attr_type(l)) {
			case DATA:
				idx = cur_idx[l]++;
				rd.attr_data(builder.mesh.attrs[l][idx], l);

				mesh.attrs[l][idx].setq([] (int q, const auto delta, const auto pred) { return pred::decodeDelta(delta, pred, q); }, mesh.attrs[l][idx], mesh.attrs[l].accu()[0]);
				lhist[a].insert(mesh.conn.org(f, le), idx);
				break;
			case HIST:
				idx = cur_idx[l] - 1 - rd.attr_ghist(l);
				lhist[a].insert(mesh.conn.org(f, le), idx);
				break;
			case LHIST:
				idx = lhist[a].find(mesh.conn.org(f, le), rd.attr_lhist(l));
				break;
			}

			builder.bind_corner_attr(f, le, a, idx);
		}
	}

	template <typename P>
	void decode(P &prog)
	{
		prog.start(order.size());
		for (int i = 0; i < order.size(); ++i) {
			mesh::conn::fepair &e = order[i];

			vtx_post(e.f(), e.e());
			prog(i);
		}
		for (int i = 0; i < builder.mesh.attrs.num_face(); ++i) {
			face_post(i, 0);
			for (int c = 0; c < mesh.conn.num_edges(i); ++c) {
				corner_post(i, c);
			}
		}
		prog.end();
	}
};

}
