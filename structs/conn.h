#pragma once

#include "types.h"
#include "faces.h"

#include <vector>
#include <unordered_map>
#include <limits>

namespace mesh {
namespace conn {

struct fepair {
	struct hash {
		std::size_t operator()(const fepair &fep) const
		{
			return std::hash<std::size_t>()(fep.ff << 4 | fep.ee);
		}
	};

	faceidx_t ff;
	ledgeidx_t ee;

	inline fepair() : ff(std::numeric_limits<faceidx_t>::max()), ee(0)
	{}
	inline fepair(int) : ff(std::numeric_limits<faceidx_t>::max()), ee(0)
	{}
	inline fepair(faceidx_t _f, ledgeidx_t _e) : ff(_f), ee(_e)
	{}

	inline faceidx_t f() const
	{
		return ff;
	}
	inline ledgeidx_t e() const
	{
		return ee;
	}

	inline bool operator==(const fepair &o) const
	{
		return o.f() == f() && o.e() == e();
	}
	inline bool operator!=(const fepair &o) const
	{
		return !(o == *this);
	}
};
inline std::ostream &operator<<(std::ostream &os, const fepair &fp)
{
	return os << "[" << fp.f() << ", " << fp.e() << "]";
}

inline ledgeidx_t en(ledgeidx_t e, ledgeidx_t c)
{
	return e + 1 == c ? 0 : e + 1;
}
inline ledgeidx_t ep(ledgeidx_t e, ledgeidx_t c)
{
	return e == 0 ? c - 1 : e - 1;
}

struct Conn {
	struct edgeorg {
		vtxidx_t org;
		fepair twin;
	};
	vtxidx_t mnum_vtx;
	faceidx_t mnum_tri;
	Faces &f;
	std::vector<edgeorg> edges;

	inline Conn(Faces &_f) : f(_f), mnum_vtx(0), mnum_tri(0)
	{}

	inline faceidx_t add_face(ledgeidx_t ne)
	{
		mnum_tri += ne - 2;
		faceidx_t idx = f.add(ne);
		edgeidx_t o = edges.size();
		edges.resize(edges.size() + ne);
		for (edgeidx_t i = 0; i < ne; ++i) {
			edges[o + i].twin = fepair(idx, i);
		}
		return idx;
	}

	inline void reserve(faceidx_t hint)
	{
		f.reserve(hint);
		edges.reserve(hint * 3);
	}

	inline faceidx_t num_face()
	{
		return f.size();
	}
	inline vtxidx_t num_vtx()
	{
		return mnum_vtx;
	}
	inline faceidx_t num_tri()
	{
		return mnum_tri;
	}
	inline ledgeidx_t num_edges(faceidx_t fi) const
	{
		return f.num_edges(fi);
	}
	inline faceidx_t face(fepair a) const
	{
		return a.f();
	}

	inline void set_org(faceidx_t fi, ledgeidx_t v, vtxidx_t o)
	{
		edges[f.off(fi) + v].org = o;
		mnum_vtx = std::max(mnum_vtx, o + 1);
	}
	inline fepair enext(fepair a) const
	{
		return fepair(a.f(), en(a.e(), num_edges(a.f())));
	}
	inline fepair eprev(fepair a) const
	{
		return fepair(a.f(), ep(a.e(), num_edges(a.f())));
	}
	inline fepair twin(fepair a) const
	{
		return edges[f.off(a.f()) + a.e()].twin;
	}
	inline vtxidx_t org(faceidx_t fi, ledgeidx_t v) const
	{
		return edges[f.off(fi) + v].org;
	}
	inline vtxidx_t org(fepair a) const
	{
		return org(a.f(), a.e());
	}
	inline vtxidx_t dest(fepair a) const
	{
		return org(enext(a));
	}
	inline void fmerge(fepair a, fepair b)
	{
		edges[f.off(a.f()) + a.e()].twin = b;
		edges[f.off(b.f()) + b.e()].twin = a;
	}
	inline void swap(fepair a, fepair b)
	{
		fmerge(edges[f.off(a.f()) + a.e()].twin, edges[f.off(b.f()) + b.e()].twin);
// 		fmerge(edges[f.off(b.f()) + b.e()].twin, edges[f.off(b.f()) + b.e()].twin);
		fmerge(a, b);
	}
};

struct Builder {
	struct pairhash {
		template <typename T, typename U>
		std::size_t operator()(const std::pair<T, U> &x) const
		{
			return std::hash<T>()(x.first) + std::hash<U>()(x.second);
		}
	};
	typedef std::pair<vtxidx_t, vtxidx_t> edgemap_e;
	typedef std::unordered_map<edgemap_e, fepair, pairhash> edgemap;

	edgemap em;
	faceidx_t cur_f;
	ledgeidx_t cur_c;
	vtxidx_t last_vtx;
	vtxidx_t start_vtx;
	bool automerge;

	Conn &c;

	inline Builder(Conn &_conn) : c(_conn), cur_f(std::numeric_limits<faceidx_t>::max()), automerge(true)
	{}

	inline void reserve(faceidx_t hint)
	{
		em.reserve(hint * 3);
		c.reserve(hint);
	}

	inline void add_edge(vtxidx_t a, vtxidx_t b)
	{
		if (!automerge) return;

		fepair p(cur_f, cur_c - 1);

		edgemap::iterator twin = em.find(edgemap_e(b, a));
		if (twin != em.end()) { // found a twin, merge
			c.fmerge(twin->second, p);
			em.erase(twin);
		} else {
			em.insert(std::make_pair(edgemap_e(a, b), p));
		}
	}
	inline faceidx_t face_begin(ledgeidx_t ne)
	{
		cur_f = c.add_face(ne);
		cur_c = 0;
		last_vtx = std::numeric_limits<vtxidx_t>::max();
		return cur_f;
	}
	inline void face_end()
	{
		add_edge(last_vtx, start_vtx);
	}
	inline void set_org(vtxidx_t vtx)
	{
		c.set_org(cur_f, cur_c, vtx);
		if (last_vtx != std::numeric_limits<vtxidx_t>::max()) add_edge(last_vtx, vtx);
		else start_vtx = vtx;
		++cur_c;
		last_vtx = vtx;
	}
};

}
}
