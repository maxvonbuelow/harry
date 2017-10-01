/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <type_traits>

namespace transform {

template <int S> struct int_t_wrapper;
template <> struct int_t_wrapper<1>  { typedef int8_t type; };
template <> struct int_t_wrapper<2> { typedef int16_t type; };
template <> struct int_t_wrapper<4> { typedef int32_t type; };
template <> struct int_t_wrapper<8> { typedef int64_t type; };
template <int S> using int_t = typename int_t_wrapper<S>::type;

template <int S> struct uint_t_wrapper;
template <> struct uint_t_wrapper<1>  { typedef uint8_t type; };
template <> struct uint_t_wrapper<2> { typedef uint16_t type; };
template <> struct uint_t_wrapper<4> { typedef uint32_t type; };
template <> struct uint_t_wrapper<8> { typedef uint64_t type; };
template <int S> using uint_t = typename uint_t_wrapper<S>::type;

template <int S> struct fp_t_wrapper;
template <> struct fp_t_wrapper<4> { typedef float type; };
template <> struct fp_t_wrapper<8> { typedef double type; };
template <int S> using fp_t = typename fp_t_wrapper<S>::type;

template <int S>
inline int_t<S> flipfloat(int_t<S> i)
{
	return i ^ -((uint_t<S>)i >> ((S << 3) - 1)) >> 1;
}

template <typename T> T zigzag_encode(T a)
{
	uint_t<sizeof(T)> c = *((uint_t<sizeof(T)>*)&a);
	uint_t<sizeof(T)> r = (c << 1) ^ (((c >> ((sizeof(T) << 3) - 1)) != 0) ? -1 : 0);
	return *((T*)&r);
}
template <typename T> T zigzag_decode(T a)
{
	uint_t<sizeof(T)> c = *((uint_t<sizeof(T)>*)&a);
	uint_t<sizeof(T)> r = (c >> 1) ^ (((c & 1) != 0) ? -1 : 0);
	return *((T*)&r);
}

template <typename T>
int_t<sizeof(T)> float2int(T f)
{
	return flipfloat<sizeof(T)>(*((int_t<sizeof(T)>*)&f));
}
template <typename T>
fp_t<sizeof(T)> int2float(T i)
{
	i = flipfloat<sizeof(T)>(i);
	return *((fp_t<sizeof(T)>*)&i);
}

template <typename T> T float2int_impl(T f, std::true_type) { int_t<sizeof(T)> r = float2int(f); return *((T*)&r); }
template <typename T> T int2float_impl(T f, std::true_type) { return int2float(*((int_t<sizeof(T)>*)&f)); }
template <typename T> T float2int_impl(T f, std::false_type) { return f; }
template <typename T> T int2float_impl(T f, std::false_type) { return f; }

template <typename T> T float2int_all(T f) { return float2int_impl(f, std::is_floating_point<T>()); }
template <typename T> T int2float_all(T f) { return int2float_impl(f, std::is_floating_point<T>()); }


template <typename T, typename U> T xor_all(T a, U b) { uint_t<sizeof(T)> r = *((uint_t<sizeof(T)>*)&a) ^ *((uint_t<sizeof(T)>*)&b); return *((T*)&r); }


template <typename T, typename U> T add64_impl(T a, U b, std::true_type) { return a + b; }
template <typename T, typename U> T sub64_impl(T a, U b, std::true_type) { return a - b; }
template <typename T, typename U> int64_t add64_impl(T a, U b, std::false_type) { return (int64_t)a + (int64_t)b; }
template <typename T, typename U> int64_t sub64_impl(T a, U b, std::false_type) { return (int64_t)a - (int64_t)b; }

template <typename T, typename U> T add64(T a, U b) { return add64_impl(a, b, std::is_floating_point<T>()); }
template <typename T, typename U> T sub64(T a, U b) { return sub64_impl(a, b, std::is_floating_point<T>()); }

#define VERT_PRED_SUB

#ifdef VERT_PRED_SUB
template <typename T, typename U> T usub_impl(T a, U b, std::true_type) { int_t<sizeof(T)> r = *((int_t<sizeof(T)>*)&a) - *((int_t<sizeof(T)>*)&b); r = zigzag_encode(r); return *((T*)&r); }
template <typename T, typename U> T uadd_impl(T a, U b, std::true_type) { int_t<sizeof(T)> r = zigzag_decode(*((int_t<sizeof(T)>*)&a)) + *((int_t<sizeof(T)>*)&b); return *((T*)&r); }

template <typename T, typename U> T usub_impl(T a, U b, std::false_type) { return a - b; }
template <typename T, typename U> T uadd_impl(T a, U b, std::false_type) { return a + b; }

template <typename T, typename U> T usub(T a, U b) { return usub_impl(a, b, std::true_type()/*std::is_floating_point<T>()*/); }
template <typename T, typename U> T uadd(T a, U b) { return uadd_impl(a, b, std::true_type()/*std::is_floating_point<T>()*/); }
#else
template <typename T, typename U> T usub(T a, U b) { uint_t<sizeof(T)> r = *((uint_t<sizeof(T)>*)&a) ^ *((uint_t<sizeof(T)>*)&b); return *((T*)&r); }
template <typename T, typename U> T uadd(T a, U b) { uint_t<sizeof(T)> r = *((uint_t<sizeof(T)>*)&a) ^ *((uint_t<sizeof(T)>*)&b); return *((T*)&r); }
#endif





template <typename T, typename U> T divround(T n, U d, std::true_type) { return n / d; }
template <typename T, typename U> T divround(T n, U d, std::false_type) { return (n + (d >> 1)) / d; }

template <typename T, typename U> T divround(T n, U d) { return divround(n, d, std::is_floating_point<T>()); }

}
