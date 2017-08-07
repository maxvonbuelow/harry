#pragma once

#include "io.h"

#include "../float2int.h"
#include "prediction.h"

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

	AbsAttrCoder(mesh::Mesh &_mesh) : mesh(_mesh), vtx_is_encoded(/*_mesh.size_vtx()*/_mesh.attrs.num_vtx(), false)
	{}

	bool calculate_parallelogram(conn::fepair e0, conn::fepair e1, conn::fepair eo, mesh::regidx_t r)
	{
		mesh::vtxidx_t v0 = mesh.org(e0), v1 = mesh.org(e1), vo = mesh.org(eo);
		if (!vtx_is_encoded[v0] || !vtx_is_encoded[v1] || !vtx_is_encoded[vo]) return false;
		regidx_t r0 = mesh.attrs.vtx2reg(v0), r1 = mesh.attrs.vtx2reg(v1), ro = mesh.attrs.vtx2reg(vo);
		if (r0 != r || r1 != r || ro != r) return false;

// 		parals.push_back(Paral{ v0, v1, vo });
		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
			ref_t as = mesh.attrs.binding_vtx(r, a);

			// fetch values
			MElm d0 = mesh.attrs[as][mesh.attrs.attr_vtx(v0, a)], d1 = mesh.attrs[as][mesh.attrs.attr_vtx(v1, a)], dop = mesh.attrs[as][mesh.attrs.attr_vtx(vo, a)];

			// increment accu
			if (mesh.attrs[as].sizesp() - 1 <= curparal) mesh.attrs[as].resizesp(curparal + 2);

			mesh.attrs[as].special(curparal + 1).setq([] (int q, const auto d0c, const auto d1c, const auto doc) { /*assert_eq(q,32);*/return pred::predict(d0c, d1c, doc, q); }, d0, d1, dop);
			mesh.attrs[as].big().set([] (const auto cur, const auto val) { /*std::cout << sizeof(val) << std::endl;*/ return cur + val; }, mesh.attrs[as].big(), mesh.attrs[as].special(curparal + 1));
		}
		++curparal;
// return false;

// if (curparal == 0) {
// 	++curparal;
		return true;
// } return false;
	}

	template <bool SAME>
	int calculate_parallelograms(conn::fepair e, mesh::regidx_t r)
	{
		int used = 0;
		if (mesh.num_edges(e.f()) == 3) {
			if (!SAME) {
				conn::fepair e0 = e, e1 = mesh.enext(e0), eo = mesh.enext(e1);
				used += calculate_parallelogram(e0, e1, eo, r) ? 1 : 0;
				return used;
			}
			conn::fepair beg = mesh.enext(e);
// 			e = mesh.sym(mesh.fnext(beg)); // TODO
			e = mesh.neigh(beg);
			int cnt = 0;
// 			while (e != beg) {
			if (e != mesh.sym(beg)) {
				used += calculate_parallelograms<false>(e, r);
// 				e = mesh.sym(mesh.fnext(e));
			}
// 				++cnt;
// 			}
// 			assert_le(cnt, 1);
			return used;
		}

		conn::fepair e0 = mesh.enext(e);
		conn::fepair e1 = mesh.sym(e);
		if (SAME) e1 = mesh.enext(e1);

		conn::fepair eol = mesh.enext(e0);
		conn::fepair eor = mesh.enext(e1);

		e1 = mesh.sym(e1);
		eor = mesh.sym(eor);

		used += calculate_parallelogram(e0, e1, eol, r) ? 1 : 0;
		used += calculate_parallelogram(e0, e1, eor, r) ? 1 : 0;

		return used;
	}

	uint64_t pars = 0;
	void vtx(faceidx_t ff, ledgeidx_t ee)
	{
		conn::fepair e(ff, ee);
		mesh::vtxidx_t v = mesh.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);
		// iterate over triangle fan
		std::stack<conn::fepair> fan;
		std::unordered_set<conn::fepair, conn::fepair::hash> seen;
		fan.push(e);
// 		int s = 0;
		int used = 0;
		curparal = 0;
		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
			MElm ee = mesh.attrs[mesh.attrs.binding_vtx(r, a)].big();
			ee.set([] (const auto) { return 0; }, ee);
		}
		while (!fan.empty()) {
			conn::fepair cur = fan.top(), beg = cur; fan.pop();
			seen.insert(cur);

			// "cur" contains the edge originating from v used to compute the parallelogramms.
			used += calculate_parallelograms<true>(cur, r);
// 			if(used)break;
// 			std::cout << mesh.org(cur) << " " << used << std::endl;
// 			++s;

			// iterate over all non-manifolds
			cur = mesh.neigh(cur);
			if (cur != mesh.sym(beg)) {
// 			do {
// 				cur = mesh.sym(mesh.fnext(cur));
				conn::fepair next = mesh.enext(cur);
				if (mesh.org(next) != v) continue; // TODO: should we account flipped triangles?
				if (seen.find(next) == seen.end()) fan.push(next);
// 			} while (cur != beg);
			}
		}
// 		std::cout << "SWAG " << used << std::endl;
// 		std::cerr << "v1.1" << std::endl;
		vtx_is_encoded[v] = true;
// 		std::cerr << "v1.2" << std::endl;
		assert_eq(curparal, used);
// 		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
// 			ref_t as = mesh.attrs.binding_vtx(r, a);
// 			MElm ee = mesh.attrs[as].special(0);
// 			ee.set([used] (auto cur) { return used == 0 ? decltype(cur)(0) : divround<decltype(cur)>(cur, used); }, ee);
// // 			wr.vtxparal(ee, as);
// 		}
// 		wr.reg_vtx(r);
		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
			ref_t as = mesh.attrs.binding_vtx(r, a);
			MElm avg = mesh.attrs[as].big();
			avg.set([used] (const auto cur) { return used == 0 ? decltype(cur)(0) : divround(cur, (decltype(cur))used); }, avg);
			bool useavg = false; // use average when at least one type is quantized
			for (int i = 0; i < mesh.attrs[as].fmt.type.size(); ++i) {
// 				useavg |= mesh.attrs[as].fmt.isquant(i);
				useavg |= mesh.attrs[as].fmt.type[i] != MixingFmt::FLOAT && mesh.attrs[as].fmt.type[i] != MixingFmt::DOUBLE;
			}
			if (useavg) curparal = 0;

			double minsumdiff = std::numeric_limits<double>::infinity();
			int minparal = -1;
			std::vector<int> best(100, -1); // TODO
			MElm bestval = mesh.attrs[as].special(0);
			bestval.set([] (const auto x) { return std::numeric_limits<decltype(x)>::max(); }, bestval);
			for (int i = 0; i < curparal; ++i) {
				MElm cur = mesh.attrs[as].special(i + 1);

				double sum = 0;
				bestval.setqi([&sum,i,&best] (int idx, int qq, const auto cu, const auto av, const auto bv) {
					decltype(cu) delta = av > cu ? av - cu : cu - av;
					if (delta < bv) {
						best[idx] = i;
						return delta;
					}
					return bv;

// 					if 

// 					decltype(cu) d = pred::encodeDelta(av, cu, qq);
// 					uint_t<sizeof(cu)> inc = (uint_t<sizeof(cu)>)*((int_t<sizeof(cu)>*)&d);
// 					if (sum + inc > sum) sum += inc/*(cu > av ? cu - av : av - cu)*/;
// 					else sum = std::numeric_limits<uint64_t>::max();
// 					return cu;
				}, cur, avg, bestval); // TODO: weighting
// 				if (sum <= minsumdiff) {
// 					minsumdiff = sum;
// 					minparal = i;
// 				}
			}
			if (curparal != 0) {
				avg.setqi([this,&as,&best] (int idx, int qq, const auto cur) { return mesh.attrs[as].special(best[idx] + 1).get<decltype(cur)>(idx); }, avg);
			} else {
			}
// 			assert_eq(minparal == -1, curparal == 0);
// 			minparal=0;
// 			if (curparal != 0) avg.set([] (auto o) { return o; }, mesh.attrs[as].special(minparal + 1)); // TODO: move this

// 			ref_t as = mesh.attrs.binding_vtx(r, a);
// 			MElm ee = curparal != 0 ? mesh.attrs[as].special(minparal + 1) : avg;
// 			avg.setq([] (uint8_t q, auto raw, auto cur) { return pred::encodeDelta(raw, cur, q); }, mesh.attrs[as][v], ee);
// 			avg.set([] (const auto cur) { return cur; }, ee);
// 			wr.vtxabs(ee, as);

// 				wr.vtxparal(avg, as);
// 			else
// 				wr.vtxparal(mesh.attrs[as].special(minparal + 1), as);
		}
// 		std::cerr << "v1.3" << std::endl;
// 		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
// 			ref_t as = mesh.attrs.binding_vtx(r, a);
// 			MElm ee = mesh.attrs[as].special(0);
// 			ee.setq([] (uint8_t q, auto raw, auto cur) { return pred::encodeDelta(raw, cur, q); }, mesh.attrs[as][v], ee);
// 			wr.vtxparal(ee, as);
// 		}

		/////////////////////////////AUTO
		
		pars += used;
// 		accu.print(std::cout); std::cout << std::endl;
		
// 		std::cout << "par size of " << v << " is " << used << " cur avg: " << (float)pars / mesh.size_vtx() << std::endl;
	}
};

template <typename WR>
struct AttrCoder : AbsAttrCoder {
	mesh::Mesh &mesh;
	WR &wr;
	std::vector<GlobalHistory> ghist;
	std::vector<LocalHistory> lhist;
// 	std::vector<conn::fepair> order;

	AttrCoder(mesh::Mesh &_mesh, WR &_wr) : mesh(_mesh), wr(_wr), AbsAttrCoder(_mesh), ghist(mesh.attrs.size()), lhist(mesh.attrs.size_wedge)
	{
		for (ref_t i = 0; i < mesh.attrs.size(); ++i) {
			ghist[i].resize(mesh.attrs[i].size());
		}
		for (offset_t i = 0; i < mesh.attrs.size_wedge; ++i) {
			lhist[i].resize(mesh.attrs.num_vtx());
		}
	}

// 	void face(mesh::faceidx_t f, mesh::ledgeidx_t le)
// 	{
// // 		conn::fepair e(f, le), beg = e;
// // 
// // 		// iterate over all vertices of the face
// // 		do {
// // 			mesh::vtxidx_t v = mesh.org(e);
// // 			if (!vtx_is_encoded[v]) vtx(e);
// // 			e = mesh.enext(e);
// // 		} while (e != beg);
// 		
// // 		order.emplace_back(f, e);
// 	}

	void vtx(mesh::faceidx_t f, mesh::ledgeidx_t le)
	{
		conn::fepair e(f, le);
		mesh::vtxidx_t v = mesh.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);

		AbsAttrCoder::vtx(f, le);
		wr.reg_vtx(r);
		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
			ref_t as = mesh.attrs.binding_vtx(r, a);
			ref_t attr_attrs = mesh.attrs.attr_vtx(v, a);
			offset_t tidx = ghist[as].get(attr_attrs);
			if (tidx == ghist[as].unset()) {
				ghist[as].set(attr_attrs);
				MElm ev = mesh.attrs[as][attr_attrs];

// 				ref_t as = mesh.attrs.binding_vtx(r, a);
				MElm eebig = mesh.attrs[as].big();
				MElm ee = mesh.attrs[as].special(0);
				ee.set([] (auto other) { return other; }, eebig);

				//std::cout << "pred: " << ee.get<float>(0) << " " << ee.get<float>(1) << " " << ee.get<float>(2); std::cout << std::endl;
	// 			std::cout << "pred: " << ee.at<uint32_t>(0) << " " << ee.at<uint32_t>(1) << " " << ee.at<uint32_t>(2); std::cout << std::endl;
				ee.setq([] (int qqqq, const auto raw, const auto cur) { /*const int qq = 32;*//*assert_eq(qqqq,32);*/ /*if (qqqq != 32) {qqqq = 32;std::cerr << "Called" << std::endl;} */return pred::encodeDelta(raw, cur, qqqq); }, mesh.attrs[as][mesh.attrs.attr_vtx(v, a)], ee);
// 				std::cout << "corr: " << ee.at<uint32_t>(0) << " " << ee.at<uint32_t>(1) << " " << ee.at<uint32_t>(2); std::cout << std::endl;
				wr.vtxabs(ee, as);
// 				std::cout << "raw: " << mesh.attrs[as][mesh.attrs.attr_vtx(v, a)].at<uint32_t>(0) << " " << mesh.attrs[as][mesh.attrs.attr_vtx(v, a)].at<uint32_t>(1) << " " << mesh.attrs[as][mesh.attrs.attr_vtx(v, a)].at<uint32_t>(2); std::cout << std::endl;

			} else {
				wr.vtxhist(tidx, as); // TODO: send negative offsets
			}



		}
	}

	void face(faceidx_t f)
	{
		regidx_t rf = mesh.attrs.face2reg(f);
		wr.reg_face(rf);
		for (offset_t a = 0; a < mesh.attrs.num_attr_face(rf); ++a) {
			ref_t as = mesh.attrs.binding_face(rf, a);
			ref_t attr_attrs = mesh.attrs.attr_face(f, a);
			offset_t tidx = ghist[as].get(attr_attrs);
			if (tidx == ghist[as].unset()) {
				ghist[as].set(attr_attrs);
				MElm ef = mesh.attrs[as][attr_attrs];
				wr.faceabs(ef, as);
			} else {
				wr.facehist(tidx, as); // TODO: send negative offsets
			}
		}
	}
	void face(faceidx_t f, faceidx_t o)
	{
		regidx_t rf = mesh.attrs.face2reg(f), ro = mesh.attrs.face2reg(o);
		wr.reg_face(rf);
		for (offset_t a = 0; a < mesh.attrs.num_attr_face(rf); ++a) {
			ref_t as = mesh.attrs.binding_face(rf, a);
			ref_t attr_attrs = mesh.attrs.attr_face(f, a);
			offset_t tidx = ghist[as].get(attr_attrs);
			if (tidx == ghist[as].unset()) {
				ghist[as].set(attr_attrs);
				MElm ef = mesh.attrs[as][attr_attrs];
				if (rf == ro) {
					MElm eo = mesh.attrs[as][mesh.attrs.attr_face(o, a)];
					MElm c = mesh.attrs[as].special(0);
					c.set([] (auto fdv, auto fdneighv) { return xor_all(float2int_all(fdv), float2int_all(fdneighv)); }, ef, eo);
					wr.facediff(c, as);
				} else {
					wr.faceabs(ef, as);
				}
			} else {
				wr.facehist(tidx, as); // TODO: send negative offsets
			}
		}
	}
	void wedge(faceidx_t f, lvtxidx_t v, vtxidx_t gv)
	{
		regidx_t rf = mesh.attrs.face2reg(f);
		for (offset_t a = 0; a < mesh.attrs.num_attr_wedge(rf); ++a) {
			ref_t as = mesh.attrs.binding_wedge(rf, a);
			ref_t attr_attrs = mesh.attrs.attr_wedge(f, v, a);
			bool lempty = lhist[a].empty(gv);
			int loff = lhist[a].insert(gv, attr_attrs);
			if (loff != -1) {
				wr.wedgelhist(loff, as);
				continue;
			}
			offset_t tidx = ghist[as].get(attr_attrs);
			if (tidx == ghist[as].unset()) {
				ghist[as].set(attr_attrs);
				MElm ew = mesh.attrs[as][attr_attrs];
				if (lempty) {
					wr.wedgeabs(ew, as);
				} else {
					MElm c = mesh.attrs[as].special(0);
					MElm eo = mesh.attrs[as][lhist[a].find(gv, 0)];
					c.set([] (auto av, auto aoldv) { return xor_all(float2int_all(av), float2int_all(aoldv)); }, ew, eo);
					// TODO
					wr.wedgediff(c, as);
				}
			} else {
				wr.wedgehist(tidx, as); // TODO: send negative offsets
			}
		}
	}

// 	struct Paral { mesh::vtxidx_t v0, v1, vo; };
// 	std::list<Paral> parals;



// 	void code()
// 	{
// 		for (int i = 0; i < order.size(); ++i) {
// 			
// 		}
// 	}
};
/*
template <typename WR>
struct AttrCoder {
	attr::Attrs &attrs;
	std::vector<GlobalHistory> ghist;
	std::vector<LocalHistory> lhist;
	WR &wr;

	AttrCoder(attr::Attrs &_attrs, WR &_wr) : attrs(_attrs), wr(_wr), ghist(attrs.size()), lhist(attrs.size_wedge)
	{
		for (ref_t i = 0; i < attrs.size(); ++i) {
			ghist[i].resize(attrs[i].size());
		}
		for (offset_t i = 0; i < attrs.size_wedge; ++i) {
			lhist[i].resize(attrs.num_vtx());
		}
	}

	void vtx(vtxidx_t v)
	{
		regidx_t rv = attrs.vtx2reg(v);
		wr.reg_vtx(rv);
		for (offset_t a = 0; a < attrs.num_attr_vtx(rv); ++a) {
			ref_t as = attrs.binding_vtx(rv, a);
			ref_t attr_attrs = attrs.attr_vtx(v, a);
			offset_t tidx = ghist[as].get(attr_attrs);
			if (tidx == ghist[as].unset()) {
				ghist[as].set(attr_attrs);
				MElm ev = attrs[as][attr_attrs];
				wr.vtxabs(ev, as);
			} else {
				wr.vtxhist(tidx, as); // TODO: send negative offsets
			}
		}
	}
	void vtx(vtxidx_t v, vtxidx_t l, vtxidx_t r, vtxidx_t o)
	{
		regidx_t rv = attrs.vtx2reg(v), rl = attrs.vtx2reg(l), rr = attrs.vtx2reg(r), ro = attrs.vtx2reg(o);
		wr.reg_vtx(rv);
		for (offset_t a = 0; a < attrs.num_attr_vtx(rv); ++a) {
			ref_t as = attrs.binding_vtx(rv, a);
			ref_t attr_attrs = attrs.attr_vtx(v, a);
			offset_t tidx = ghist[as].get(attr_attrs);
			if (tidx == ghist[as].unset()) {
				ghist[as].set(attr_attrs);
				MElm ev = attrs[as][attr_attrs];

				if (rv == rl && rv == rr && rv == ro) {
					// same regions: use parallelogramm
					MElm el = attrs[as][attrs.attr_vtx(l, a)], er = attrs[as][attrs.attr_vtx(r, a)], eo = attrs[as][attrs.attr_vtx(o, a)];
					MElm c = attrs[as].special(0);
// 					c.set([] (auto e0v, auto e1v, auto vtv, auto vv) { return usub(float2int_all(vv, sizeof(vv) << 3), float2int_all((decltype(vv))add64(e1v, sub64(e0v, vtv)), sizeof(vv) << 3)); }, el, er, eo, ev); // TODO: This generate signed values, even if they were unsigned
					c.setq([] (uint8_t q, auto e0v, auto e1v, auto vtv, auto vv) { return pred::encode(vv, e0v, e1v, vtv, q); }, el, er, eo, ev);
// 					c.set([] (auto e0v, auto e1v, auto vtv, auto vv) { return usub(float2int_all(vv), float2int_all(vtv)); }, el, er, eo, ev); // TODO: This generate signed values, even if they were unsigned
// 					c.enforce_quant();
					wr.vtxparal(c, as);
				} else {
					// different regions
					wr.vtxabs(ev, as);
				}
			} else {
				wr.vtxhist(tidx, as); // TODO: send negative offsets
			}
		}
	}

};*/

template <typename RD>
struct AttrDecoder : AbsAttrCoder {
// 	attr::Attrs &attrs;
// 	std::vector<LocalHistory> lhist;
	RD &rd;
	std::vector<ref_t> cur_idx;

	mesh::Mesh &mesh;
// 	WR &wr;
	std::vector<conn::fepair> order;
// 	std::vector<bool> vtx_is_encoded;


	
	AttrDecoder(mesh::Mesh &_mesh, RD &_rd) : mesh(_mesh), rd(_rd), AbsAttrCoder(_mesh)/*, ghist(attrs.size())*//*, lhist(attrs.size_wedge)*/, cur_idx(_mesh.attrs.size(), 0)
	{
// 		std::cout << "ilctrorsf " << mesh.attrs.size() << std::endl;
// 		for (offset_t i = 0; i < attrs.size_wedge; ++i) {
// 			lhist[i].resize(attrs.num_vtx());
// 		}
	}

	void vtx(faceidx_t f, ledgeidx_t le)
	{
		conn::fepair e(f, le);
		order.push_back(e);
		vtxidx_t v = mesh.org(e);
// 		std::cout << v<< std::endl;

		// save delta to store
		regidx_t rv = mesh.attrs.vtx2reg(v) = rd.reg_vtx();
		mesh.attrs.add_vtx();
		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(rv); ++a) {
			ref_t as = mesh.attrs.binding_vtx(rv, a);
			ref_t attr_attrs;
			attr_attrs = cur_idx[as]++;
			rd.vtxabs(mesh.attrs[as][attr_attrs], as);

			mesh.attrs.attr_vtx(v, a) = attr_attrs;
		}
	}

	void vtx_post(faceidx_t f, ledgeidx_t le)
	{
		conn::fepair e(f, le);
		mesh::vtxidx_t v = mesh.org(e);
		mesh::regidx_t r = mesh.attrs.vtx2reg(v);

// 		std::cerr << "v1" << std::endl;
		AbsAttrCoder::vtx(f, le);
// 		std::cerr << "v2" << std::endl;
		for (offset_t a = 0; a < mesh.attrs.num_attr_vtx(r); ++a) {
			ref_t as = mesh.attrs.binding_vtx(r, a);
			MElm ee = mesh.attrs[as].special(0); // TODO: big
			std::cout << "pred: " << ee.at<uint32_t>(0) << " " << ee.at<uint32_t>(1) << " " << ee.at<uint32_t>(2); std::cout << std::endl;
// 			std::cout << "pred: " << ee.at<uint32_t>(0) << " " << ee.at<uint32_t>(1) << " " << ee.at<uint32_t>(2); std::cout << std::endl;
			ref_t attr_attrs = mesh.attrs.attr_vtx(v, a);
			assert_lt(attr_attrs, mesh.attrs[as].size());
			std::cout << "corr: " << mesh.attrs[as][attr_attrs].at<uint32_t>(0) << " " << mesh.attrs[as][attr_attrs].at<uint32_t>(1) << " " << mesh.attrs[as][attr_attrs].at<uint32_t>(2); std::cout << std::endl;
			mesh.attrs[as][attr_attrs].setq([] (int q, const auto delta, const auto pred) { /*assert_eq(q,32);*/return pred::decodeDelta(delta, pred, q); }, mesh.attrs[as][attr_attrs], ee);
			std::cout << "raw: " << mesh.attrs[as][attr_attrs].at<uint32_t>(0) << " " << mesh.attrs[as][attr_attrs].at<uint32_t>(1) << " " << mesh.attrs[as][attr_attrs].at<uint32_t>(2); std::cout << std::endl;
		}
	}

	void decode()
	{
		for (int i = 0; i < order.size(); ++i) {
			conn::fepair &e = order[i];

			vtx_post(e.f(), e.e());
		}
	}

// 
// 	void vtx(vtxidx_t v)
// 	{
// 		regidx_t rv = attrs.vtx2reg(v) = rd.reg_vtx();
// 		attrs.add_vtx();
// 		for (offset_t a = 0; a < attrs.num_attr_vtx(rv); ++a) {
// 			ref_t as = attrs.binding_vtx(rv, a);
// 			ref_t attr_attrs;
// 			switch (rd.attr_type(as)) {
// 			case DATA:
// 				attr_attrs = cur_idx[as]++;
// 				rd.vtxabs(attrs[as][attr_attrs], as);
// 				break;
// 			case HIST:
// 				attr_attrs = rd.vtxhist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_vtx(v, a) = attr_attrs;
// 		}
// 	}
// 	void vtx(vtxidx_t v, vtxidx_t l, vtxidx_t r, vtxidx_t o)
// 	{
// 		regidx_t rv = attrs.vtx2reg(v) = rd.reg_vtx(), rl = attrs.vtx2reg(l), rr = attrs.vtx2reg(r), ro = attrs.vtx2reg(o);
// 		attrs.add_vtx();
// // 			std::cout << "YOOOO---" << std::endl;
// 		for (offset_t a = 0; a < attrs.num_attr_vtx(rv); ++a) {
// 			ref_t as = attrs.binding_vtx(rv, a);
// 			ref_t attr_attrs;
// // 			std::cout << "YOOOO" << std::endl;
// 			switch (rd.attr_type(as)) {
// 			case DATA: {
// 				attr_attrs = cur_idx[as]++;
// 				MElm ev = attrs[as][attr_attrs];
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
// 				attr_attrs = rd.vtxhist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_vtx(v, a) = attr_attrs;
// 		}
// 
// 	}
// 	void face(faceidx_t f, ledgeidx_t ne)
// 	{
// 		regidx_t rf = attrs.face2reg(f) = rd.reg_face();
// 		attrs.add_face_ne(ne);
// 		for (offset_t a = 0; a < attrs.num_attr_face(rf); ++a) {
// 			ref_t as = attrs.binding_face(rf, a);
// 			ref_t attr_attrs;
// 			switch (rd.attr_type(as)) {
// 			case DATA:
// 				attr_attrs = cur_idx[as]++;
// 				rd.faceabs(attrs[as][attr_attrs], as);
// 				break;
// 			case HIST:
// 				attr_attrs = rd.facehist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_face(f, a) = attr_attrs;
// 		}
// 	}
// 	void face(faceidx_t f, faceidx_t o, ledgeidx_t ne)
// 	{
// 		regidx_t rf = attrs.face2reg(f) = rd.reg_face(), ro = attrs.face2reg(o);
// 		attrs.add_face_ne(ne); // TODO
// 		for (offset_t a = 0; a < attrs.num_attr_face(rf); ++a) {
// 			ref_t as = attrs.binding_face(rf, a);
// 			ref_t attr_attrs;
// 			switch (rd.attr_type(as)) {
// 			case DATA: {
// 				attr_attrs = cur_idx[as]++;
// 				MElm ef = attrs[as][attr_attrs];
// 				if (rf == ro) {
// 					MElm eo = attrs[as][attrs.attr_face(o, a)];
// 					rd.facediff(ef, as);
// 					ef.set([] (auto err, auto vneighv) { return int2float_all(xor_all(err, float2int_all(vneighv, sizeof(vneighv) << 3))); }, ef, eo);
// 				} else {
// 					rd.faceabs(ef, as);
// 				}
// 				break;}
// 			case HIST:
// 				attr_attrs = rd.facehist(as);
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// 			attrs.attr_face(f, a) = attr_attrs;
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
// 			ref_t attr_attrs;
// 			switch (rd.attr_type(as)) {
// 			case DATA: {
// 				attr_attrs = cur_idx[as]++;
// // 				std::cout << attr_attrs << " " << attrs[as].size() << std::endl;
// 				MElm ew = attrs[as][attr_attrs];
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
// 				lhist[a].insert(gv, attr_attrs);
// 				break;}
// 			case HIST:
// 				attr_attrs = rd.wedgehist(as);
// 				lhist[a].insert(gv, attr_attrs);
// 				break;
// 			case LHIST:
// 				attr_attrs = lhist[a].find(gv, rd.wedgelhist(as));
// 				break;
// 			default:
// 				// TODO: error
// 				;
// 			}
// // 			std::cout << "f: " << f << " v: " << v << " a: " << a << " val: " << attr_attrs << std::endl;
// 			attrs.attr_wedge(f, v, a) = attr_attrs;
// // 			std::cout << "sf3" << std::endl;
// 		}
// // 		std::cout << "finsf" << std::endl;
// 	}
};

}
