#pragma once

#include <vector>

#include "types.h"

namespace mesh {

struct Faces {
	std::vector<edgeidx_t> offsets;
	std::vector<char> have_edges;
	struct EdgeIterator {
		const char *edges;
		int size;
		int i;

		bool operator==(const EdgeIterator &it) const
		{
			return **this == *it;
		}
		bool operator!=(const EdgeIterator &it) const
		{
			return !(*this == it);
		}

		void seek()
		{
			while (i < size && !edges[i]) ++i;
		}

		EdgeIterator &operator++()
		{
			++i;
			seek();
			return *this;
		}
		int operator*() const
		{
			return i;
		}
	};

	Faces() : offsets(1, 0)
	{}

	edgeidx_t size()
	{
		return offsets.size() - 1;
	}
	void seen_edge(ledgeidx_t ne)
	{
		if (ne >= have_edges.size()) have_edges.resize(ne + 1, false);
		have_edges[ne] = true;
	}
	faceidx_t add(ledgeidx_t ne)
	{
		faceidx_t idx = size();
		offsets.push_back(offsets.back() + ne);
		seen_edge(ne);
		return idx;
	}
	edgeidx_t off(faceidx_t f) const
	{
		return offsets[f];
	}
	ledgeidx_t num_edges(faceidx_t f) const
	{
		return offsets[f + 1] - offsets[f];
	}
	edgeidx_t size_edge()
	{
		return offsets.back();
	}
	void reserve(faceidx_t hint)
	{
		offsets.reserve(hint);
	}

	EdgeIterator edge_begin()
	{
		EdgeIterator ei = { have_edges.data(), have_edges.size(), 0 };
		ei.seek();
		return ei;
	}
	EdgeIterator edge_end()
	{
		return EdgeIterator{ NULL, have_edges.size(), have_edges.size() };
	}
};

}
