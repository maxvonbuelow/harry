/*
 * Copyright (C) 2017, Max von Buelow
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

/*
 * Implementation of the Fenwick Tree.
 *
 * Related publications:
 * Fenwick, Peter M. "A new data structure for cumulative frequency tables." Software: Practice and Experience 24.3 (1994): 327-336.
 * Fenwick, P. A New Data sturcture for cumulative Probability Tables: an Improved Frequency to Symbol Algorithm. Department of Computer Science, The University of Auckland, New Zealand, 1995.
 */

#include <vector>
#include <stdint.h>
#include "msb.h"

namespace arith {

// Implementation of a Fenwick Tree
template <typename TF = uint64_t, typename TS = uint32_t, typename TC = uint32_t>
struct AdaptiveStatisticsModule {
	typedef TF FreqType;
	typedef TS SymType;
	typedef TC CountType;

	static const int b = sizeof(TF) * 8;
	static const int f = b - 2;
	static const TF FFULL = TF(1) << f;

	std::vector<TF> F, C;
	TC n;
	TS mid;

	AdaptiveStatisticsModule(TC _n = 256) : F(_n, 0), C(_n, 0), n(_n), mid(msb(_n))
	{}

	AdaptiveStatisticsModule(const AdaptiveStatisticsModule&) = delete;
	AdaptiveStatisticsModule &operator=(const AdaptiveStatisticsModule&) = delete;

	void range(TS s, TF &l, TF &h)
	{
		h = cumulative(s);
		l = h - C[s];
	}
	TF total() const
	{
		return cumulative(n - 1);
	}
	TS symbol(TF target, TF &l, TF &h)
	{
		TS s = 0;
		TF original = target;
		if (original >= F[0]) {
			TS cur_mid = mid;
			while (cur_mid > 0) {
				if (s + cur_mid <= n && F[s + cur_mid - 1] <= target) {
					target -= F[s + cur_mid - 1];
					s += cur_mid;
				}
				cur_mid >>= 1;
			}
		}
		l = original - target;
		h = l + C[s];
		return s;
	}
	void init(TS s, TF incr = 1)
	{
		inc(s, incr);
	}
	void inc(TS s, TF inc = 1)
	{
		inc_impl(s, inc);

		if (total() > FFULL) halve();
	}
	TF frequency(TS s) const
	{
		return C[s];
	}
	void set(TS s, TF f)
	{
		inc_impl(s, -frequency(s) + f);
	}
	void halve()
	{
// 		std::exit(1);
		for (TS i = 0; i < n; ++i) {
			inc_impl(i, -(frequency(i) >> 1));
		}
	}
	TF cumulative(TS s) const
	{
		TS i = s + 1;
		TF h = 0;
		while (i != 0) {
			h += F[i - 1];
			i = backward(i);
		}
		return h;
	}

private:
	static TS forward(TS i)
	{
		return i + (i & -i);
	}
	static TS backward(TS i)
	{
		return i - (i & -i);
	}
	void inc_impl(TS s, TF inc)
	{
		TS i = s + 1;
		while (i <= n) {
			F[i - 1] += inc;
			i = forward(i);
		}
		C[s] += inc;
	}
};

}
