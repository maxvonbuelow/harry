/*
 * Copyright (C) 2017, Max von Buelow
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

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
