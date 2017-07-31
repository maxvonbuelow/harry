#pragma once

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

	AdaptiveStatisticsModule(TC _n) : F(_n, 0), C(_n, 0), n(_n), mid(msb(_n))
	{}

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
