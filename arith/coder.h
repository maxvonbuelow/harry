#pragma once

#include <stdint.h>
#include <algorithm>
#include <istream>
#include <ostream>

#include "bitstream.h"

#include <fstream> // TODO

namespace arith {

template <typename TF = uint64_t>
struct Coder {
	typedef TF FreqType;

	static const int b = sizeof(TF) * 8;
	static const TF HALF = TF(1) << (b - 1);
	static const TF QUARTER = TF(1) << (b - 2);
};

template <typename TF = uint64_t, typename TBO = uint64_t>
struct Encoder : Coder<TF> {
	using Coder<TF>::b;
	using Coder<TF>::HALF;
	using Coder<TF>::QUARTER;

	TF L, R; // L = low, R = range
	TBO bits_outstanding;
	bitostream os;
	bool flushed;

	Encoder(std::ostream &_os) : os(_os), L(0), R(HALF), bits_outstanding(0), flushed(false)
	{}

	~Encoder()
	{
		flush();
	}

	Encoder(const Encoder&) = delete;
	Encoder &operator=(const Encoder&) = delete;

	void flush()
	{
		if (flushed) return;
		flushed = true;

		for (int i = b - 1; i >= 0; --i) {
			bit_plus_follow((L >> i) & 1);
		}
		os.flush();
	}

// 	std::ofstream oss = std::ofstream("arith.dbg");
	void operator()(TF l, TF h, TF t)
	{
// 		oss << l << " " << h << " " << t << std::endl;
		TF r = R / t;
		L = L + r * l;
		if (h < t)
			R = r * (h - l);
		else
			R = R - r * l;

		while (R <= QUARTER) {
			if (L <= HALF && L + R <= HALF) {
				bit_plus_follow(0);
			} else if (L >= HALF) {
				bit_plus_follow(1);
				L -= HALF;
			} else {
				++bits_outstanding;
				L -= QUARTER;
			}
			L *= 2;
			R *= 2;
		}
	}
	template <typename S>
	void operator()(S &freq, typename S::SymType s)
	{
		TF l, h, t = freq.total();
		freq.range(s, l, h);
		(*this)(l, h, t);
	}

private:
	void write_one_bit(unsigned char bit)
	{
		os << bit;
	}
	void bit_plus_follow(unsigned char x)
	{
		write_one_bit(x);
		while (bits_outstanding > 0) {
			write_one_bit(!x);
			--bits_outstanding;
		}
	}
};

template <typename TF = uint64_t>
struct Decoder : Coder<TF> {
	using Coder<TF>::b;
	using Coder<TF>::HALF;
	using Coder<TF>::QUARTER;

	TF R, D, r; // R = range
	bitistream is;

// 	std::ifstream iss = std::ifstream("arith.dbg");
	Decoder(std::istream &_is) : is(_is), R(HALF), D(0)
	{
		for (int i = 0; i < b; ++i) {
			D = 2 * D + read_one_bit();
		}
	}

	Decoder(const Decoder&) = delete;
	Decoder &operator=(const Decoder&) = delete;

	TF decode_target(TF t)
	{
		r = R / t;
		return std::min(t - 1, D / r);
	}

	int lineno = 0;
	void operator()(TF l, TF h, TF t)
	{
		++lineno;
// 		TF ll, hh, tt;
// 		iss >> ll >> hh >> tt;
// 		if (ll != l || hh != h || tt != t) std::cout << "(INFO) Line: " << lineno << std::endl;
// 		assert_eq(ll, l); assert_eq(hh, h); assert_eq(tt, t);
		// r already set by decode_target
		D = D - r * l;
		if (h < t)
			R = r * (h - l);
		else
			R = R - r * l;

		while (R <= QUARTER) {
			R *= 2;
			D = 2 * D + read_one_bit();
		}
	}
	template <typename S>
	typename S::SymType operator()(S &freq)
	{
		TF l, h, t = freq.total();
		TF target = decode_target(t);
		typename S::SymType s = freq.symbol(target, l, h);
		(*this)(l, h, t);
		return s;
	}


private:
	unsigned char read_one_bit()
	{
		unsigned char bit;
		is >> bit;
		return bit;
	}
};

}
