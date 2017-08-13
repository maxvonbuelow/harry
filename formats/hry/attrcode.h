#pragma once

#include "io.h"

#include "transform.h"
#include "prediction.h"

#include <thread> // TODO
#include <chrono> // TODO

namespace attrcode {

// using namespace mesh;
// using namespace attr;
// 
// struct GlobalHistory {
// 	std::vector<offset_t> attr_transmitted;
// 	offset_t cur_tidx;
// 
// 	GlobalHistory() : cur_tidx(0)
// 	{}
// 
// 	void resize(offset_t size)
// 	{
// 		attr_transmitted.resize(size, unset());
// 	}
// 
// 	void set(offset_t idx)
// 	{
// 		attr_transmitted[idx] = cur_tidx++;
// 	}
// 	offset_t get(offset_t idx)
// 	{
// 		return attr_transmitted[idx];
// 	}
// 	offset_t unset()
// 	{
// 		return std::numeric_limits<offset_t>::max();
// 	}
// };
// 
// 
// struct LocalHistory {
// 	std::vector<std::vector<ref_t>> hist; // TODO: this results in a vector<vector<vector<X>>>, reduce this to max 2 containers
// 
// 	void resize(vtxidx_t size)
// 	{
// 		hist.resize(size);
// 	}
// 
// 	bool empty(vtxidx_t v)
// 	{
// 		return hist[v].empty();
// 	}
// 
// 	int insert(vtxidx_t v, ref_t a)
// 	{
// 		for (int i = 0; i < hist[v].size(); ++i) {
// 			if (hist[v][i] == a) return i;
// 		}
// 		hist[v].push_back(a);
// 		return -1;
// 	}
// 	ref_t find(vtxidx_t v, int off)
// 	{
// 		return hist[v][off];
// 	}
// };

struct AbsAttrCoder {
	std::vector<bool> vtx_is_encoded;
	int curparal;
	mesh::Mesh &mesh;

	AbsAttrCoder(mesh::Mesh &_mesh) : mesh(_mesh), vtx_is_encoded(_mesh.attrs.num_vtx(), false)
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
		if (mesh.conn.num_edges(e.f()) > 4) // when polygon is a pentagon or more, we have two parallelograms
			use_paral(mesh.conn.org(e0), mesh.conn.org(e1), mesh.conn.org(mesh.conn.eprev(e)), r);
	}
	void tfan(mesh::conn::fepair ein, mesh::regidx_t r, mesh::vtxidx_t debug)
	{
		mesh::conn::fepair e = ein, t;
		do {
			paral(e, r);
			t = mesh.conn.twin(e);
			if (t == e) goto BWD;
			e = mesh.conn.enext(t);
			assert_eq(debug, mesh.conn.org(e));
		} while (e != ein);
		return;

BWD:
		e = mesh.conn.eprev(ein);
		t = mesh.conn.twin(e);
		if (e == t) return;
		e = t;
		assert_eq(debug, mesh.conn.org(e));
		do {
			paral(e, r);
			e = mesh.conn.eprev(e);
			t = mesh.conn.twin(e);
			if (e == t) break;
			e = t;
			assert_eq(debug, mesh.conn.org(e));
		} while (e != ein);
	}

	void vtx(mesh::faceidx_t ff, mesh::ledgeidx_t ee, bool in = false)
	{
		// TODO: ULONG and LONG must handled seperately: div first add then
		mesh::conn::fepair e(ff, ee);
		mesh::vtxidx_t v = mesh.conn.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);

		curparal = 0;
// 		std::cout << "sf1?" << std::endl;
		tfan(e, r, v);
// 		std::cout << "sf2?" << std::endl;
		vtx_is_encoded[v] = true;
		int num_paral = curparal;

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_vtxlist(r, a);

			// compute average
			mixing::View avg = mesh.attrs[l].big()[0];
			avg.set([] (const auto) { return 0; }, avg);
			for (int i = 0; i < num_paral; ++i) {
				avg.sets([] (const auto cur, const auto val) { return cur + val; }, avg, mesh.attrs[l].cache()[i]);
			}
			avg.set([num_paral] (const auto cur) { return num_paral == 0 ? decltype(cur)(0) : transform::divround(cur, (decltype(cur))num_paral); }, avg);

			// selection (without quantization or integral values: average; with quantization: value closest to quantization)
			mixing::View res = mesh.attrs[l].accu()[0];
			if (num_paral == 0) {
				res.set([] (const auto x) { return decltype(x)(0); }, res);
			} else {
				res.set([] (const auto x) { return std::numeric_limits<decltype(x)>::max(); }, res);
				for (int i = 0; i < num_paral; ++i) {
					res.setst([] (mixing::Type t, const auto resv, const auto predv, const auto avgv) {
						if (t != mixing::DOUBLE && t != mixing::FLOAT) return avgv;

						decltype(resv) resdiff = avgv > resv ? avgv - resv : resv - avgv;
						decltype(predv) preddiff = avgv > predv ? avgv - predv : predv - avgv;
						return resdiff < preddiff ? resv : predv;
					}, res, mesh.attrs[l].cache()[i], avg);
				}
			}
		}
	}
};

template <typename WR>
struct AttrCoder : AbsAttrCoder {
	mesh::Mesh &mesh;
	WR &wr;
// 	std::vector<GlobalHistory> ghist;
// 	std::vector<LocalHistory> lhist;
	std::vector<mesh::conn::fepair> order;

	AttrCoder(mesh::Mesh &_mesh, WR &_wr) : mesh(_mesh), wr(_wr), AbsAttrCoder(_mesh)/*, ghist(mesh.attrs.size()), lhist(mesh.attrs.size_wedge)*/
	{
// 		for (ref_t i = 0; i < mesh.attrs.size(); ++i) {
// 			ghist[i].resize(mesh.attrs[i].size());
// 		}
// 		for (offset_t i = 0; i < mesh.attrs.size_wedge; ++i) {
// 			lhist[i].resize(mesh.attrs.num_vtx());
// 		}
	}

	void vtx(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		order.push_back(e);
	}
	void vtx_post(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		mesh::conn::fepair e(f, le);
		mesh::vtxidx_t v = mesh.conn.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);

		AbsAttrCoder::vtx(f, le, true);
		wr.reg_vtx(r);

		for (mesh::listidx_t a = 0; a < mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = mesh.attrs.binding_reg_vtxlist(r, a);

			mixing::View res = mesh.attrs[l].accu()[0];
			res.setq([] (int q, const auto raw, const auto pred) { return pred::encodeDelta(raw, pred, q); }, mesh.attrs[l][mesh.attrs.binding_vtx_attr(v, a)], res);
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
		prog.end();
	}

// 	void face(faceidx_t f)
// 	{
// 		regidx_t rf = mesh.attrs.face2reg(f);
// 		wr.reg_face(rf);
// 		for (offset_t a = 0; a < mesh.attrs.num_attr_face(rf); ++a) {
// 			ref_t as = mesh.attrs.binding_face(rf, a);
// 			ref_t attridx = mesh.attrs.attr_face(f, a);
// 			offset_t tidx = ghist[as].get(attridx);
// 			if (tidx == ghist[as].unset()) {
// 				ghist[as].set(attridx);
// 				MElm ef = mesh.attrs[as][attridx];
// 				wr.faceabs(ef, as);
// 			} else {
// 				wr.facehist(tidx, as); // TODO: send negative offsets
// 			}
// 		}
// 	}
// 	void face(faceidx_t f, faceidx_t o)
// 	{
// 		regidx_t rf = mesh.attrs.face2reg(f), ro = mesh.attrs.face2reg(o);
// 		wr.reg_face(rf);
// 		for (offset_t a = 0; a < mesh.attrs.num_attr_face(rf); ++a) {
// 			ref_t as = mesh.attrs.binding_face(rf, a);
// 			ref_t attridx = mesh.attrs.attr_face(f, a);
// 			offset_t tidx = ghist[as].get(attridx);
// 			if (tidx == ghist[as].unset()) {
// 				ghist[as].set(attridx);
// 				MElm ef = mesh.attrs[as][attridx];
// 				if (rf == ro) {
// 					MElm eo = mesh.attrs[as][mesh.attrs.attr_face(o, a)];
// 					MElm c = mesh.attrs[as].special(0);
// 					c.set([] (auto fdv, auto fdneighv) { return xor_all(float2int_all(fdv), float2int_all(fdneighv)); }, ef, eo);
// 					wr.facediff(c, as);
// 				} else {
// 					wr.faceabs(ef, as);
// 				}
// 			} else {
// 				wr.facehist(tidx, as); // TODO: send negative offsets
// 			}
// 		}
// 	}
// 	void wedge(faceidx_t f, lvtxidx_t v, vtxidx_t gv)
// 	{
// 		regidx_t rf = mesh.attrs.face2reg(f);
// 		for (offset_t a = 0; a < mesh.attrs.num_attr_wedge(rf); ++a) {
// 			ref_t as = mesh.attrs.binding_wedge(rf, a);
// 			ref_t attridx = mesh.attrs.attr_wedge(f, v, a);
// 			bool lempty = lhist[a].empty(gv);
// 			int loff = lhist[a].insert(gv, attridx);
// 			if (loff != -1) {
// 				wr.wedgelhist(loff, as);
// 				continue;
// 			}
// 			offset_t tidx = ghist[as].get(attridx);
// 			if (tidx == ghist[as].unset()) {
// 				ghist[as].set(attridx);
// 				MElm ew = mesh.attrs[as][attridx];
// 				if (lempty) {
// 					wr.wedgeabs(ew, as);
// 				} else {
// 					MElm c = mesh.attrs[as].special(0);
// 					MElm eo = mesh.attrs[as][lhist[a].find(gv, 0)];
// 					c.set([] (auto av, auto aoldv) { return xor_all(float2int_all(av), float2int_all(aoldv)); }, ew, eo);
// 					// TODO
// 					wr.wedgediff(c, as);
// 				}
// 			} else {
// 				wr.wedgehist(tidx, as); // TODO: send negative offsets
// 			}
// 		}
// 	}
};


template <typename RD>
struct AttrDecoder : AbsAttrCoder {
// 	std::vector<LocalHistory> lhist;
	RD &rd;
	std::vector<mesh::attridx_t> cur_idx;

	mesh::Builder &builder;
	std::vector<mesh::conn::fepair> order;


	
	AttrDecoder(mesh::Builder &_builder, RD &_rd) : builder(_builder), rd(_rd), AbsAttrCoder(_builder.mesh)/*, ghist(attrs.size())*//*, lhist(attrs.size_wedge)*/, cur_idx(_builder.mesh.attrs.size(), 0)
	{
// 		for (offset_t i = 0; i < attrs.size_wedge; ++i) {
// 			lhist[i].resize(attrs.num_vtx());
// 		}
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

		AbsAttrCoder::vtx(f, le);

		mesh::regidx_t r = rd.reg_vtx();
		builder.vtx_reg(v, r);
		for (mesh::listidx_t a = 0; a < builder.mesh.attrs.num_bindings_vtx_reg(r); ++a) {
			mesh::listidx_t l = builder.mesh.attrs.binding_reg_vtxlist(r, a);

			mesh::attridx_t attridx = cur_idx[l]++;
			rd.attr_data(builder.mesh.attrs[l][attridx], l);

			builder.bind_vtx_attr(v, a, attridx);

			mixing::View pred = mesh.attrs[l].accu()[0];
			mesh.attrs[l][attridx].setq([] (int q, const auto delta, const auto pred) { return pred::decodeDelta(delta, pred, q); }, mesh.attrs[l][attridx], pred);
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
		prog.end();
	}

// 
// 	void vtx(vtxidx_t v)
// 	{
// 		regidx_t rv = attrs.vtx2reg(v) = rd.reg_vtx();
// 		attrs.add_vtx();
// 		for (offset_t a = 0; a < attrs.num_attr_vtx(rv); ++a) {
// 			ref_t as = attrs.binding_vtx(rv, a);
// 			ref_t attridx;
// 			switch (rd.attr_type(as)) {
// 			case DATA:
// 				attridx = cur_idx[as]++;
// 				rd.vtxabs(attrs[as][attridx], as);
// 				break;
// 			case HIST:
// 				attridx = rd.vtxhist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_vtx(v, a) = attridx;
// 		}
// 	}
// 	void vtx(vtxidx_t v, vtxidx_t l, vtxidx_t r, vtxidx_t o)
// 	{
// 		regidx_t rv = attrs.vtx2reg(v) = rd.reg_vtx(), rl = attrs.vtx2reg(l), rr = attrs.vtx2reg(r), ro = attrs.vtx2reg(o);
// 		attrs.add_vtx();
// // 			std::cout << "YOOOO---" << std::endl;
// 		for (offset_t a = 0; a < attrs.num_attr_vtx(rv); ++a) {
// 			ref_t as = attrs.binding_vtx(rv, a);
// 			ref_t attridx;
// // 			std::cout << "YOOOO" << std::endl;
// 			switch (rd.attr_type(as)) {
// 			case DATA: {
// 				attridx = cur_idx[as]++;
// 				MElm ev = attrs[as][attridx];
// 				if (rv == rl && rv == rr && rv == ro) {
// 					// same regions: use parallelogramm
// 					MElm el = attrs[as][attrs.attr_vtx(l, a)], er = attrs[as][attrs.attr_vtx(r, a)], eo = attrs[as][attrs.attr_vtx(o, a)];
// 					rd.vtxparal(ev, as);
// // 					ev.set([] (auto e0v, auto e1v, auto vtv, auto err) { return int2float_all(uadd(err, float2int_all(add64(e1v, sub64(e0v, vtv)), sizeof(err) << 3))); }
// 					ev.setq([] (uint8_t q, auto e0v, auto e1v, auto vtv, auto err) { return pred::decode(err, e0v, e1v, vtv, q); }, el, er, eo, ev);
// 				} else {
// 					// different regions
// 					rd.vtxabs(ev, as);
// 				}
// 				break;}
// 			case HIST:
// 				attridx = rd.vtxhist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_vtx(v, a) = attridx;
// 		}
// 
// 	}
// 	void face(faceidx_t f, ledgeidx_t ne)
// 	{
// 		regidx_t rf = attrs.face2reg(f) = rd.reg_face();
// 		attrs.add_face_ne(ne);
// 		for (offset_t a = 0; a < attrs.num_attr_face(rf); ++a) {
// 			ref_t as = attrs.binding_face(rf, a);
// 			ref_t attridx;
// 			switch (rd.attr_type(as)) {
// 			case DATA:
// 				attridx = cur_idx[as]++;
// 				rd.faceabs(attrs[as][attridx], as);
// 				break;
// 			case HIST:
// 				attridx = rd.facehist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_face(f, a) = attridx;
// 		}
// 	}
// 	void face(faceidx_t f, faceidx_t o, ledgeidx_t ne)
// 	{
// 		regidx_t rf = attrs.face2reg(f) = rd.reg_face(), ro = attrs.face2reg(o);
// 		attrs.add_face_ne(ne); // TODO
// 		for (offset_t a = 0; a < attrs.num_attr_face(rf); ++a) {
// 			ref_t as = attrs.binding_face(rf, a);
// 			ref_t attridx;
// 			switch (rd.attr_type(as)) {
// 			case DATA: {
// 				attridx = cur_idx[as]++;
// 				MElm ef = attrs[as][attridx];
// 				if (rf == ro) {
// 					MElm eo = attrs[as][attrs.attr_face(o, a)];
// 					rd.facediff(ef, as);
// 					ef.set([] (auto err, auto vneighv) { return int2float_all(xor_all(err, float2int_all(vneighv, sizeof(vneighv) << 3))); }, ef, eo);
// 				} else {
// 					rd.faceabs(ef, as);
// 				}
// 				break;}
// 			case HIST:
// 				attridx = rd.facehist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_face(f, a) = attridx;
// 		}
// 	}
// 	void wedge(faceidx_t f, lvtxidx_t v, vtxidx_t gv)
// 	{
// // 		std::cout << "sf1" << std::endl;
// 		regidx_t rf = attrs.face2reg(f); // already set by face(...)
// 		for (offset_t a = 0; a < attrs.num_attr_wedge(rf); ++a) {
// // 			std::cout << a << " " << lhist.size() << std::endl;
// // 			std::cout << "sf2" << std::endl;
// 			ref_t as = attrs.binding_wedge(rf, a);
// 			ref_t attridx;
// 			switch (rd.attr_type(as)) {
// 			case DATA: {
// 				attridx = cur_idx[as]++;
// // 				std::cout << attridx << " " << attrs[as].size() << std::endl;
// 				MElm ew = attrs[as][attridx];
// 				if (lhist[a].empty(gv)) {
// 					rd.wedgeabs(ew, as);
// 				} else {
// 					MElm eo = attrs[as][lhist[a].find(gv, 0)];
// 					rd.wedgediff(ew, as);
// 					// encoder:
// // 					c.set([] (auto av, auto aoldv) { return xor_all(float2int_all(av, sizeof(av) << 3), float2int_all(aoldv, sizeof(av) << 3)); }, ew, eo);
// 
// 					ew.set([] (auto err, auto aoldv) { return int2float_all(xor_all(err, float2int_all(aoldv, sizeof(err) << 3))); }, ew, eo);
// 					// TODO
// 				}
// 				lhist[a].insert(gv, attridx);
// 				break;}
// 			case HIST:
// 				attridx = rd.wedgehist(as);
// 				lhist[a].insert(gv, attridx);
// 				break;
// 			case LHIST:
// 				attridx = lhist[a].find(gv, rd.wedgelhist(as));
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// // 			std::cout << "f: " << f << " v: " << v << " a: " << a << " val: " << attridx << std::endl;
// 			attrs.attr_wedge(f, v, a) = attridx;
// // 			std::cout << "sf3" << std::endl;
// 		}
// // 		std::cout << "finsf" << std::endl;
// 	}
};

}
