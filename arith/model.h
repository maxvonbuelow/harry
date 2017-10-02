/*
 * Copyright (C) 2017, Max von Buelow
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include "coder.h"

namespace arith {

template <typename TF = uint64_t>
struct Model {
	virtual void enc(Encoder<TF> &coder, const unsigned char *s, int n) = 0;
	virtual void dec(Decoder<TF> &coder, unsigned char *s, int n) = 0;

	template <typename T>
	void encode(Encoder<TF> &coder, const T &s)
	{
		enc(coder, (const unsigned char*)&s, sizeof(T));
	}
	template <typename T>
	T decode(Decoder<TF> &coder)
	{
		T s;
		dec(coder, (unsigned char*)&s, sizeof(T));
		return s;
	}
};

template <typename T, typename S, typename TF = uint64_t>
struct ModelMult : Model<TF> {
	S stats[sizeof(T)];

	ModelMult(bool init = true)
	{
		if (init) {
			for (uint64_t i = 0; i < sizeof(T); ++i) {
				for (uint64_t j = 0; j < 256; ++j) {
					stats[i].init(j);
				}
			}
		}
	}

	void init(T val)
	{
		const char *s = (const char*)&val;
		for (int i = 0; i < sizeof(T); ++i) {
			stats[i].init(s[i]);
		}
	}

	void enc(Encoder<TF> &coder, const unsigned char *s, int n)
	{
		assert_eq(sizeof(T), n);
		for (int i = 0; i < sizeof(T); ++i) {
			coder(this->stats[i], s[i]);
			this->stats[i].inc(s[i]);
		}
	}

	void dec(Decoder<TF> &coder, unsigned char *s, int n)
	{
		assert_eq(sizeof(T), n);
		for (int i = 0; i < sizeof(T); ++i) {
			s[i] = coder(this->stats[i]);
			this->stats[i].inc(s[i]);
		}
	}
};

}
