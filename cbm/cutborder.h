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

#include <deque>
#include <list>
#include <vector>

#include "base.h"

#include "../assert.h"

namespace cbm {

template <typename T, typename V>
struct DataTpl : T {
	V idx;

	DataTpl() : idx(std::numeric_limits<V>::max())
	{}
	template <typename ...Params>
	DataTpl(V i, Params &&...params) : idx(i), T(std::forward<Params>(params)...)
	{}

	bool isUndefined()
	{
		return idx == std::numeric_limits<V>::max();
	}
};
template <typename T, typename V>
std::ostream &operator<<(std::ostream &os, const DataTpl<T, V> &d)
{
	return os << d.idx << " [" << (T)d << "]";
}

template <typename T, typename V>
struct CutBorder {
	typedef DataTpl<T, V> Data;
	typedef std::list<Data> Elements;
	struct Part : Elements {
		bool isEdgeBegin;
		Part() : isEdgeBegin(true)
		{}

		std::size_t num_edges()
		{
			return this->size() - (isEdgeBegin ? 0 : 1);
		}
	};
	typedef std::deque<Part> Parts;
	Parts parts;
	Data *first, *second;

	// acceleration structure for fast lookup if a vertex is currently on the cutboder
	std::vector<unsigned char> vertices;

	CutBorder(V num_vtx = 0) : vertices(num_vtx, 0)
	{}

	Part &cur_part()
	{
		return parts.back();
	}

	bool atEnd()
	{
		return parts.empty();
	}

	void traverseStep(Data &v0, Data &v1)
	{
		Part &part = cur_part();
		v0 = part.back();
		v1 = part.front();
	}

	const Data &left() // previous on cut-border
	{
		return *(++cur_part().rbegin());
	}
	const Data &right() // next on cut-border
	{
		return cur_part().front();
	}

	void activate_vertex(V i)
	{
		++vertices[i];
	}
	void deactivate_vertex(V i)
	{
		--vertices[i];
	}
	bool on_cut_border(V i)
	{
		assert_ge(vertices[i], 0);
		return vertices[i] != 0;
	}

	typename Elements::iterator get_element(int i, int p = 0)
	{
		Part &part = parts[parts.size() - 1 - p];

		if (i > 0)
			return std::next(part.begin(), i - 1);
		else
			return std::prev(part.end(), -i + 1);
	}
	void find_element(Data v, int &i, int &p)
	{
		// search vertex in both directions
		typename Parts::reverse_iterator part = parts.rbegin();
		typename Elements::reverse_iterator l = part->rbegin();
		typename Elements::iterator r = part->begin();

		i = 0; p = 0;
		while (1) {
			if (r->idx == v.idx) {
				++i;
				return;
			} else if (l->idx == v.idx) {
				i = -i;
				return;
			}

			if (std::prev(l.base()) == r || l.base() == r) {
				++p;
				++part;
				assert(part != parts.rend());
				r = part->begin();
				l = part->rbegin();
				i = 0;
			} else {
				++r; ++l;
				++i;
			}
		}
	}

	void initial(Data v0, Data v1, Data v2)
	{
		parts.emplace_back();
		Part &part = cur_part();
		part.push_back(v0); activate_vertex(v0.idx);
		part.push_back(v1); activate_vertex(v1.idx);
		part.push_back(v2); activate_vertex(v2.idx);
	}

	void newVertex(Data v)
	{
		Part &part = cur_part();
		first = &part.back();
		part.push_back(v); activate_vertex(v.idx);
		second = &part.back();
	}
	Data connectForward(OP &op)
	{
		Part &part = cur_part();
		Data d = *(++part.begin());
		if (!part.isEdgeBegin) {
			op = border();
			return Data();
		} else if (istri()) {
			typename Elements::iterator it = part.begin();
			deactivate_vertex((it++)->idx);
			deactivate_vertex((it++)->idx);
			deactivate_vertex((it++)->idx);

			parts.pop_back();
			op = CLOSE;
		} else {
			deactivate_vertex(part.front().idx);
			part.pop_front();

			op = CONNFWD;
			first = &cur_part().back();
		}
		return d;
	}
	Data connectBackward(OP &op)
	{
		Part &part = cur_part();

		// NOTE: border and close operations are always renamed to connect forward
		deactivate_vertex(part.back().idx);
		part.pop_back();

		op = CONNBWD;
		first = &part.back();

		return part.back();
	}

	bool istri()
	{
		Part &part = cur_part();
		return part.num_edges() == 3 && part.size() == 3;
	}

	OP border()
	{
		Part &part = cur_part();
		if (part.num_edges() == 1) {
			assert_eq(part.size(), 2);
			typename Elements::iterator it = part.begin();
			deactivate_vertex((it++)->idx);
			deactivate_vertex((it++)->idx);
			parts.pop_back();
		} else {
			Data endvtx = part.back();

			bool rename = !part.isEdgeBegin;

			deactivate_vertex(part.back().idx);
			part.pop_back();

			if (!part.isEdgeBegin) {
				deactivate_vertex(part.front().idx);
				part.pop_front();
			}

			part.push_front(endvtx); activate_vertex(endvtx.idx);
			part.isEdgeBegin = false;

			if (rename) return CONNFWD;
		}

		return BORDER;
	}

	Data splitCutBorder(int i)
	{
		Part &part = cur_part();
		typename Elements::iterator it = get_element(i);
		Data gate = part.back();
		deactivate_vertex(gate.idx);
		part.pop_back();

		parts.emplace_back();
		Part &newpart = cur_part();
		newpart.splice(newpart.begin(), part, part.begin(), it);
		part.push_back(gate); activate_vertex(gate.idx);
		newpart.push_back(*it); activate_vertex(it->idx);
		std::swap(part.isEdgeBegin, newpart.isEdgeBegin);

		second = &newpart.back();
		first = &part.back();

		return *it;
	}
	Data cutBorderUnion(int i, int p)
	{
		Part &part = cur_part();
		typename Elements::iterator it = get_element(i, p);
		Data gate = part.back();
		deactivate_vertex(gate.idx);
		part.pop_back();

		Part &otherpart = parts[parts.size() - 1 - p];
		part.push_back(gate); activate_vertex(gate.idx);
		first = &part.back();
		part.splice(part.end(), otherpart, it, otherpart.end());
		part.splice(part.end(), otherpart, otherpart.begin(), otherpart.end()); // it is now end
		part.push_back(*it); activate_vertex(it->idx);
		second = &part.back();

		// move part to front
		for (typename Parts::iterator it = parts.begin() + (parts.size() - 1 - p); it < parts.end() - 1; ++it) {
			it->swap(it[1]);
			std::swap(it->isEdgeBegin, it[1].isEdgeBegin);
		}
		parts.pop_back();

		return *it;
	}

	bool findAndUpdate(Data v, int &i, int &p, OP &op)
	{
		if (!on_cut_border(v.idx)) return false;
		find_element(v, i, p);
		assert_eq(get_element(i, p)->idx, v.idx);

		if (p > 0) {
			op = UNION;
			Data res = cutBorderUnion(i, p);
			assert_eq(res.idx, v.idx);
		} else {
			Part &part = cur_part();
			if (part.isEdgeBegin && (++part.begin())->idx == v.idx) {
				connectForward(op);
			} else if ((++part.rbegin())->idx == v.idx) {
				connectBackward(op);
			} else {
				op = SPLIT;
				Data res = splitCutBorder(i);
				assert_eq(res.idx, v.idx);
			}
		}
		return true;
	}
};

}
