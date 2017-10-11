/*
 * Copyright (C) 2017, Stefan Guthe
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <algorithm>
#include <type_traits>

#include "transform.h"
#include "utils/types.h"

namespace hry {
namespace pred {

template <typename T>
int quant2bits(int q)
{
	return q == 0 ? sizeof(T) << 3 : q;
}

template <class T>
T mask(int bits)
{
	return bits == (sizeof(T) << 3) ? T(-1) : T((1 << bits) - 1);
}

static const uint64_t masks[] = { 0x80ull, 0x8000ull, 0, 0x80000000ull, 0, 0, 0, 0x8000000000000000ull };

template <typename T>
uint_t<sizeof(T)> int2uint(int_t<sizeof(T)> val)
{
	return ((uint_t<sizeof(T)>)val) ^ masks[sizeof(T)];
}
template <typename T>
int_t<sizeof(T)> uint2int(uint_t<sizeof(T)> val)
{
	return ((int_t<sizeof(T)>)val) ^ masks[sizeof(T)];
}

template <class T>
T decodeDelta(const T delta, const T pred, const int bits, std::false_type)
{
	const T delta_mask[2] = { T(0), ~T(0) };
	const T max_pos = mask<T>(bits) - pred;
	// this is a corner case where the whole range is positive only
	if (pred == T(0)) return delta;

	const T balanced_max = std::min(T(pred - T(1)), max_pos);
	if ((delta >> 1) > balanced_max) {
		if (max_pos >= pred) {
			return pred + delta - balanced_max - T(1);
		} else {
			return pred - delta + balanced_max;
		}
	}
	return pred + (T(delta >> 1) ^ delta_mask[delta & 1]);
}
template <class T>
T decodeDelta(const T delta, const T pred, const int bits, std::true_type)
{
	int_t<sizeof(T)> predint = transform::float2int(pred);
	uint_t<sizeof(T)> deltaint = *((uint_t<sizeof(T)>*)&delta);
	int_t<sizeof(T)> res = uint2int<T>(decodeDelta(deltaint, int2uint<T>(predint), bits, std::false_type()));
	T res2 = transform::int2float(res);
	return res2;
}

template <class T>
T decodeDelta(const T delta, const T pred, const int q)
{
	return decodeDelta(delta, pred, quant2bits<T>(q), std::is_floating_point<T>());
}

 
template <class T>
T encodeDelta(const T raw, const T pred,  int bits, std::false_type)
{
	const T max_pos = mask<T>(bits) - pred;
	// this is a corner case where the whole range is positive only
	if (pred == T(0)) return raw;

	const T balanced_max = std::min(T(pred), max_pos);
	if (raw < pred) {
		const T dlt = pred - raw;
		if (dlt > balanced_max) return dlt + balanced_max;
		// dlt can never be 0 here
		return T(dlt << 1) - 1;
	} else {
		const T dlt = raw - pred;
		if (dlt > balanced_max) return dlt + balanced_max;
		return T(dlt << 1);
	}
}

template <class T>
T encodeDelta(const T raw, const T pred, const int bits, std::true_type)
{
	int_t<sizeof(T)> predint = transform::float2int(pred);
	int_t<sizeof(T)> rawint = transform::float2int(raw);
	uint_t<sizeof(T)> res = encodeDelta(int2uint<T>(rawint), int2uint<T>(predint), bits, std::false_type());
	T r = *((T*)&res);

#ifdef HAVE_ASSERT
	assert_eq(raw, decodeDelta(r, pred, bits));
#endif
	
	return r;
}
template <class T>
T encodeDelta(const T raw, const T pred, const int q)
{
	return encodeDelta(raw, pred, quant2bits<T>(q), std::is_floating_point<T>());
}
 
template <class T>
T predict(const T v0, const T v1, const T v2, const int bits, std::false_type)
{
	const T max = mask<T>(bits);
	if (v1 < v2) {
		// v0 - (v2 - v1)
		const T tmp = v2 - v1;
		if (tmp > v0) return T(0);
		else return v0 - tmp;
	} else {
		// v0 + (v1 - v2)
		const T tmp = v1 - v2;
		const T v = v0 + tmp;
		if ((v > max) || (v < v0)) return max;
		return v;
	}
}
template <class T>
T predict(const T v0, const T v1, const T v2, const int bits, std::true_type)
{
	return v0 + (v1 - v2);
}
template <class T>
T predict(const T v0, const T v1, const T v2, const int q)
{
	return predict(v0, v1, v2, quant2bits<T>(q), std::is_floating_point<T>());
}

template <typename T>
T predict_face(const T v0, const int q, std::false_type)
{
	return v0;
}
template <typename T>
T predict_face(const T v0, const int q, std::true_type)
{
	return v0;
}

template <typename T>
T predict_face(const T v0, const int q)
{
	return predict_face(v0, q, std::is_floating_point<T>());
}

}
}
