/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include "utils/types.h"
#include "mixing.h"

#include <type_traits>
#include <limits>
#include <vector>

namespace quant {

struct Quant {
	mesh::listidx_t l;
	int o;
	int q;

	Quant(mesh::listidx_t _l, int _o, int _q) : l(_l), o(_o), q(_q)
	{}
};

inline void set_bounds(mesh::attr::Attr &attr)
{
	attr.min().set([] (auto dummy) { return std::numeric_limits<decltype(dummy)>::max(); }, attr.min());
	attr.max().set([] (auto dummy) { return std::numeric_limits<decltype(dummy)>::min(); }, attr.max());
	for (mesh::attridx_t i = 0; i < attr.size(); ++i) {
		attr.min().set([] (auto cur, auto elem) { return elem < cur ? elem : cur; }, attr.min(), attr[i]);
		attr.max().set([] (auto cur, auto elem) { return elem > cur ? elem : cur; }, attr.max(), attr[i]);
	}
}
inline void set_bounds(mesh::attr::Attrs &attrs)
{
	for (mesh::listidx_t i = 0; i < attrs.size(); ++i) {
		set_bounds(attrs[i]);
	}
}

inline void set_scale(mesh::attr::Attr &attr)
{
	mixing::View min = attr.min(), max = attr.max();
	mixing::View spre = attr.scale_tmp(), s = attr.scale();
	spre.set([] (auto mi, auto ma) { return ma - mi; }, min, max);

	s.set([] (auto dummy) { return std::numeric_limits<decltype(dummy)>::min(); }, s);

	std::vector<int> scale_perm(attr.bounds().fmt().size());
	for (int i = 0; i < attr.interps().size(); ++i) {
		for (int j = 0; j < attr.interps().len(i); ++j) {
			scale_perm[attr.interps().off(i) + j] = attr.interps().off(i);
		}
	}
	// get max
	for (int j = 0; j < attr.bounds().fmt().size(); ++j) {
		int k = scale_perm[j];
		switch (attr.bounds().fmt().type(k)) {
		case mixing::FLOAT:  s.at<float>(k)    = std::max(s.at<float>(k)   , spre.get<float>(j));    break;
		case mixing::DOUBLE: s.at<double>(k)   = std::max(s.at<double>(k)  , spre.get<double>(j));   break;
		case mixing::ULONG:  s.at<uint64_t>(k) = std::max(s.at<uint64_t>(k), spre.get<uint64_t>(j)); break;
		case mixing::LONG:   s.at<int64_t>(k)  = std::max(s.at<int64_t>(k) , spre.get<int64_t>(j));  break;
		case mixing::UINT:   s.at<uint32_t>(k) = std::max(s.at<uint32_t>(k), spre.get<uint32_t>(j)); break;
		case mixing::INT:    s.at<int32_t>(k)  = std::max(s.at<int32_t>(k) , spre.get<int32_t>(j));  break;
		case mixing::USHORT: s.at<uint16_t>(k) = std::max(s.at<uint16_t>(k), spre.get<uint16_t>(j)); break;
		case mixing::SHORT:  s.at<int16_t>(k)  = std::max(s.at<int16_t>(k) , spre.get<int16_t>(j));  break;
		case mixing::UCHAR:  s.at<uint8_t>(k)  = std::max(s.at<uint8_t>(k) , spre.get<uint8_t>(j));  break;
		case mixing::CHAR:   s.at<int8_t>(k)   = std::max(s.at<int8_t>(k)  , spre.get<int8_t>(j));   break;
		}
	}
	// expand
	for (int j = 0; j < attr.bounds().fmt().size(); ++j) {
		int k = scale_perm[j];
		switch (attr.bounds().fmt().type(j)) {
		case mixing::FLOAT:  s.at<float>(j)    = s.get<float>(k);    break;
		case mixing::DOUBLE: s.at<double>(j)   = s.get<double>(k);   break;
		case mixing::ULONG:  s.at<uint64_t>(j) = s.get<uint64_t>(k); break;
		case mixing::LONG:   s.at<int64_t>(j)  = s.get<int64_t>(k);  break;
		case mixing::UINT:   s.at<uint32_t>(j) = s.get<uint32_t>(k); break;
		case mixing::INT:    s.at<int32_t>(j)  = s.get<int32_t>(k);  break;
		case mixing::USHORT: s.at<uint16_t>(j) = s.get<uint16_t>(k); break;
		case mixing::SHORT:  s.at<int16_t>(j)  = s.get<int16_t>(k);  break;
		case mixing::UCHAR:  s.at<uint8_t>(j)  = s.get<uint8_t>(k);  break;
		case mixing::CHAR:   s.at<int8_t>(j)   = s.get<int8_t>(k);   break;
		}
	}
// 	std::cout << "Scale: "; spre.print(std::cout); std::cout << std::endl;
// 	std::cout << "Min: "; min.print(std::cout); std::cout << std::endl;
// 	std::cout << "Max: "; max.print(std::cout); std::cout << std::endl;
// 	std::cout << "New scale: "; s.print(std::cout); std::cout << std::endl;
}

template <typename T>
T rescale(T val, T from, T to, std::true_type)
{
	return val / from * to;
}
template <typename T>
T rescale(T val, T from, T to, std::false_type)
{
	return val / from * to + val % from * to / from;
}
template <typename T>
T rescale(T val, T from, T to)
{
	return rescale<T>(val, from, to, std::is_floating_point<T>());
}

inline void requant(mixing::View src, mixing::View min, mixing::View scale, mixing::View dst)
{
	for (int j = 0; j < src.fmt.size(); ++j) {
		bool srcq = src.fmt.isquant(j), dstq = dst.fmt.isquant(j);
		if (!srcq && !dstq) {
			std::copy(src.data(j), src.data(j) + src.bytes(j), dst.data(j));
			continue;
		}

		uint64_t q;
		if (srcq) {
			switch (src.fmt.stype(j)) {
			case mixing::ULONG:  q = src.at<uint64_t>(j); break;
			case mixing::UINT:   q = src.at<uint32_t>(j); break;
			case mixing::USHORT: q = src.at<uint16_t>(j); break;
			case mixing::UCHAR:  q = src.at<uint8_t>(j);  break;
			default: throw std::runtime_error("Invalid quantization type");
			}
		} else {
			switch (src.fmt.stype(j)) {
			case mixing::FLOAT:
				q = rescale<float>(src.at<float>(j) - min.at<float>(j), scale.at<float>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1) + 0.5f;
				break;
			case mixing::DOUBLE:
				q = rescale<double>(src.at<double>(j) - min.at<double>(j), scale.at<double>(j), (1 << (uint64_t)dst.fmt.quant(j)) - 1) + 0.5;
				break;
			case mixing::ULONG:
				q = rescale<uint64_t>(src.at<uint64_t>(j) - min.at<uint64_t>(j), scale.at<uint64_t>(j), (1 << (uint64_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::LONG:
				q = rescale<int64_t>(src.at<int64_t>(j) - min.at<int64_t>(j), scale.at<int64_t>(j), (1 << (uint64_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::UINT:
				q = rescale<uint32_t>(src.at<uint32_t>(j) - min.at<uint32_t>(j), scale.at<uint32_t>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::INT:
				q = rescale<int32_t>(src.at<int32_t>(j) - min.at<int32_t>(j), scale.at<int32_t>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::USHORT:
				q = rescale<uint16_t>(src.at<uint16_t>(j) - min.at<uint16_t>(j), scale.at<uint16_t>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::SHORT:
				q = rescale<int16_t>(src.at<int16_t>(j) - min.at<int16_t>(j), scale.at<int16_t>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::UCHAR:
				q = rescale<uint8_t>(src.at<uint8_t>(j) - min.at<uint8_t>(j), scale.at<uint8_t>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1);
				break;
			case mixing::CHAR:
				q = rescale<int8_t>(src.at<int8_t>(j) - min.at<int8_t>(j), scale.at<int8_t>(j), (1 << (uint32_t)dst.fmt.quant(j)) - 1);
				break;
			}
		}

		if (srcq && dstq) {
			q = rescale<uint64_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, (1 << (uint32_t)dst.fmt.quant(j)) - 1);
		}

		if (dstq) {
			switch (dst.fmt.stype(j)) {
			case mixing::ULONG:  dst.at<uint64_t>(j) = q; break;
			case mixing::UINT:   dst.at<uint32_t>(j) = q; break;
			case mixing::USHORT: dst.at<uint16_t>(j) = q; break;
			case mixing::UCHAR:  dst.at<uint8_t>(j)  = q; break;
			default: throw std::runtime_error("Invalid quantization type");
			}
		} else {
			switch (dst.fmt.stype(j)) {
			case mixing::FLOAT:
				dst.at<float>(j) = rescale<float>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<float>(j)) + min.at<float>(j);
				break;
			case mixing::DOUBLE:
				dst.at<double>(j) = rescale<double>(q, (1 << (uint64_t)src.fmt.quant(j)) - 1, scale.at<double>(j)) + min.at<double>(j);
				break;
			case mixing::ULONG:
				dst.at<uint64_t>(j) = rescale<uint64_t>(q, (1 << (uint64_t)src.fmt.quant(j)) - 1, scale.at<uint64_t>(j)) + min.at<uint64_t>(j);
				break;
			case mixing::LONG:
				dst.at<int64_t>(j) = rescale<int64_t>(q, (1 << (uint64_t)src.fmt.quant(j)) - 1, scale.at<int64_t>(j)) + min.at<int64_t>(j);
				break;
			case mixing::UINT:
				dst.at<uint32_t>(j) = rescale<uint32_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<uint32_t>(j)) + min.at<uint32_t>(j);
				break;
			case mixing::INT:
				dst.at<int32_t>(j) = rescale<int32_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<int32_t>(j)) + min.at<int32_t>(j);
				break;
			case mixing::USHORT:
				dst.at<uint16_t>(j) = rescale<uint16_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<uint16_t>(j)) + min.at<uint16_t>(j);
				break;
			case mixing::SHORT:
				dst.at<int16_t>(j) = rescale<int16_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<int16_t>(j)) + min.at<int16_t>(j);
				break;
			case mixing::UCHAR:
				dst.at<uint8_t>(j) = rescale<uint8_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<uint8_t>(j)) + min.at<uint8_t>(j);
				break;
			case mixing::CHAR:
				dst.at<int8_t>(j) = rescale<int8_t>(q, (1 << (uint32_t)src.fmt.quant(j)) - 1, scale.at<int8_t>(j)) + min.at<int8_t>(j);
				break;
			}
		}
	}
}
inline void requant(mesh::attr::Attr &attr, const mixing::Fmt &fmt)
{
	set_scale(attr);
	for (mesh::attridx_t j = 0; j < attr.size(); ++j) {
		requant(attr[j], attr.min(), attr.scale(), attr.at(j, fmt));
	}
}
inline void requant(mesh::attr::Attrs &attrs, const std::vector<Quant> &quant, bool clear)
{
	for (mesh::listidx_t l = 0; l < attrs.size(); ++l) {
		attrs[l].backup_fmt();
	}
	if (clear) {
		for (mesh::listidx_t l = 0; l < attrs.size(); ++l) {
			for (int i = 0; i < attrs[l].fmt().size(); ++i) {
				attrs[l].tmp().setquant(i, 0);
			}
		}
	}
	for (int i = 0; i < quant.size(); ++i) {
		Quant q = quant[i];
		attrs[q.l].tmp().setquant(q.o, q.q);
	}
	for (mesh::listidx_t l = 0; l < attrs.size(); ++l) {
		requant(attrs[l], attrs[l].tmp());
		attrs[l].restore_fmt();
	}
}

}
