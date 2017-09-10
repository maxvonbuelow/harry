#pragma once

#include <vector>
#include <iostream>

#include "types.h"
#include "faces.h"
#include "mixing.h"

namespace mesh {
namespace attr {

enum Target { FACE, VTX, CORNER, NONE };

struct Attr : mixing::Array {
	mixing::Array maccu;
	mixing::Array mcache;
	mixing::Array mbig;
	mixing::Array mbounds;
	Target target;
	mixing::Interps minterps;

	Attr(const mixing::Fmt &fmt, const mixing::Interps &_interps, Target &_target) : mixing::Array(fmt), maccu(fmt), mcache(fmt), mbig(fmt.big()), mbounds(fmt.dequantized()), minterps(_interps), target(_target)
	{
		big().resize(1);
		accu().resize(2);
		bounds().resize(3);
	}

	mixing::Interps &interps()
	{
		return minterps;
	}

	mixing::Array &accu()
	{
		return maccu;
	}
	mixing::Array &cache()
	{
		return mcache;
	}
	mixing::Array &big()
	{
		return mbig;
	}
	mixing::Array &bounds()
	{
		return mbounds;
	}

	mixing::View min()
	{
		return bounds()[0];
	}
	mixing::View max()
	{
		return bounds()[1];
	}
	mixing::View scale()
	{
		return bounds()[2];
	}

	void finalize()
	{
		std::vector<int> quant;
		if (target == VTX) {
			quant.resize(this->fmt().size(), 14);
		}
		caclulate_scale();
// 		quantize(quant);
	}
	void finalize_rev()
	{
		caclulate_scale();
// 		dequantize();
	}
	void finalize(std::size_t i)
	{
		min().set([] (auto cur, auto elem) { return elem < cur ? elem : cur; }, min(), (*this)[i]);
		max().set([] (auto cur, auto elem) { return elem > cur ? elem : cur; }, max(), (*this)[i]);
	}

	void caclulate_scale()
	{
		mixing::View min = this->min(), max = this->max();
		mixing::View spre = accu()[0];
		spre.set([] (auto mi, auto ma) { return ma - mi; }, min, max);
// 		std::cout << "Scale: "; spre.print(std::cout); std::cout << std::endl;
// 		std::cout << "Min: "; min.print(std::cout); std::cout << std::endl;
// 		std::cout << "Max: "; max.print(std::cout); std::cout << std::endl;

		mixing::View s = scale();
		s.set([] (auto dummy) { return std::numeric_limits<decltype(dummy)>::min(); }, s);

		std::vector<int> scale_perm(this->fmt().size());
		for (int i = 0; i < interps().size(); ++i) {
			for (int j = 0; j < interps().len(i); ++j) {
				scale_perm[interps().off(i) + j] = interps().off(i);
			}
		}
		// get max
		for (int j = 0; j < this->fmt().size(); ++j) {
			int k = scale_perm[j];
			switch (this->fmt().type(k)) {
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
		for (int j = 0; j < this->fmt().size(); ++j) {
			int k = scale_perm[j];
			switch (this->fmt().type(j)) {
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
// 		std::cout << "New scale: "; s.print(std::cout); std::cout << std::endl;
	}

	void quantize(const std::vector<int> &quant)
	{
		mixing::Fmt quantized_fmt;
		for (int j = 0; j < this->fmt().size(); ++j) {
			quantized_fmt.add(this->fmt().type(j), quant[j]);
		}

		for (int i = 0; i < size(); ++i) {
			mixing::View cur = (*this)[i];
			mixing::View nf(this->data() + i * quantized_fmt.bytes(), quantized_fmt);
			for (int j = 0; j < this->fmt().size(); ++j) {
				if (!quantized_fmt.isquant(j)) {
						std::cerr << "Unimplemented" << std::endl;
						std::exit(1);
					switch (this->fmt().stype(j)) {
					case mixing::FLOAT:  nf.at<float>(j)    = cur.at<float>(j);    break;
					case mixing::DOUBLE: nf.at<double>(j)   = cur.at<double>(j);   break;
					case mixing::ULONG:  nf.at<uint64_t>(j) = cur.at<uint64_t>(j); break;
					case mixing::LONG:   nf.at<int64_t>(j)  = cur.at<int64_t>(j);  break;
					case mixing::UINT:   nf.at<uint32_t>(j) = cur.at<uint32_t>(j); break;
					case mixing::INT:    nf.at<int32_t>(j)  = cur.at<int32_t>(j);  break;
					case mixing::USHORT: nf.at<uint16_t>(j) = cur.at<uint16_t>(j); break;
					case mixing::SHORT:  nf.at<int16_t>(j)  = cur.at<int16_t>(j);  break;
					case mixing::UCHAR:  nf.at<uint8_t>(j)  = cur.at<uint8_t>(j);  break;
					case mixing::CHAR:   nf.at<int8_t>(j)   = cur.at<int8_t>(j);   break;
					}
				} else {
					int64_t q;
					switch (this->fmt().stype(j)) {
					case mixing::FLOAT:
						q = (cur.at<float>(j) - min().at<float>(j)) / scale().at<float>(j) * ((1 << (uint64_t)quantized_fmt.quant(j)) - 1) + 0.5f;
						break;
					default:
						std::cerr << "Unimplemented" << std::endl;
						std::exit(1);
// 					case MixingFmt::DOUBLE:
// 						q = (cur.at<double>(j) - min.at<double>(j)) / s.at<double>(j) * ((1 << (uint64_t)newfmt.quant[j]) - 1) + 0.5;
// 						break;
// 					case MixingFmt::UINT:
// 						q = (int64_t)(cur.at<uint32_t>(j) - min.at<uint32_t>(j)) * ((1 << (uint64_t)newfmt.quant[j]) - 1) / s.at<uint32_t>(j); // TODO: 0.5
// 						break;
// 					case MixingFmt::INT:
// 						q = (int64_t)(cur.at<int32_t>(j) - min.at<int32_t>(j)) * ((1 << (uint64_t)newfmt.quant[j]) - 1) / s.at<int32_t>(j);
// 						break;
// 					case MixingFmt::USHORT:
// 						q = (int32_t)(cur.at<uint16_t>(j) - min.at<uint16_t>(j)) * ((1 << (uint32_t)newfmt.quant[j]) - 1) / s.at<uint16_t>(j);
// 						break;
// 					case MixingFmt::SHORT:
// 						q = (int32_t)(cur.at<int16_t>(j) - min.at<int16_t>(j)) * ((1 << (uint32_t)newfmt.quant[j]) - 1) / s.at<int16_t>(j);
// 						break;
// 					case MixingFmt::UCHAR:
// 						q = (int32_t)(cur.at<uint8_t>(j) - min.at<uint8_t>(j)) * ((1 << (uint32_t)newfmt.quant[j]) - 1) / s.at<uint8_t>(j);
// 						break;
// 					case MixingFmt::CHAR:
// 						q = (int32_t)(cur.at<int8_t>(j) - min.at<int8_t>(j)) * ((1 << (uint32_t)newfmt.quant[j]) - 1) / s.at<int8_t>(j);
// 						break;
					}
					switch (quantized_fmt.stype(j)) {
					case mixing::UINT:   nf.at<uint32_t>(j) = q; break;
					case mixing::USHORT: nf.at<uint16_t>(j) = q; break;
					case mixing::UCHAR:  nf.at<uint8_t>(j)  = q; break;
					}
				}
			}
		}
		std::cout << "Shrinked store from " << this->fmt().bytes() << " to " << quantized_fmt.bytes() << std::endl;
		this->set_fmt(quantized_fmt);
		maccu.set_fmt(quantized_fmt);
		mcache.set_fmt(quantized_fmt);
		mbig.set_fmt(quantized_fmt.big());
	}

	void dequantize()
	{
// 		for (int i = 0; i < size(); ++i) {
// 			MixingView cur = (*this)[i];
// 			MixingView nf(data() + i * fmt_minmax.bytes(), fmt_minmax, interp);
// // 			std::cout << fmt_minmax.bytes() << " " << fmt.bytes() << std::endl;
// 			for (int j = 0; j < fmt.type.size(); ++j) {
// 				if (!fmt.isquant(j)) continue;
// 				int64_t q;
// 				switch (fmt.type[j]) {
// 					case MixingFmt::UINT:   q = cur.at<uint32_t>(j); break;
// 					case MixingFmt::USHORT: q = cur.at<uint16_t>(j); break;
// 					case MixingFmt::UCHAR:  q = cur.at<uint8_t>(j);  break;
// 					default: throw std::runtime_error("Invalid quantization type");
// 				}
// 				switch (fmt.orig[j]) {
// 				case MixingFmt::FLOAT:
// // 					std::cout << (int)fmt.quant[j] << std::endl;
// // 					std::cout << "scale: " << s.at<float>(j) << std::endl;
// 					nf.at<float>(j) = (float)q / ((1 << (uint32_t)fmt.quant[j]) - 1) * s.at<float>(j) + min.at<float>(j);
// 					break;
// 				case MixingFmt::DOUBLE:
// 					throw std::runtime_error("Unimpl [quant]"); // TODO: I currently don't have time for this
// 					break;
// 				case MixingFmt::UINT:
// 					throw std::runtime_error("Unimpl [quant]");
// 					break;
// 				case MixingFmt::INT:
// 					throw std::runtime_error("Unimpl [quant]");
// 					break;
// 				case MixingFmt::USHORT:
// 					throw std::runtime_error("Unimpl [quant]");
// 					break;
// 				case MixingFmt::SHORT:
// 					throw std::runtime_error("Unimpl [quant]");
// 					break;
// 				case MixingFmt::UCHAR:
// 					throw std::runtime_error("Unimpl [quant]");
// 					break;
// 				case MixingFmt::CHAR:
// 					throw std::runtime_error("Unimpl [quant]");
// 					break;
// 				}
// 			}
// 		}
// 		fmt = fmt_minmax;
	}
};

struct Bindings {
	// Assignments face/vtx to region.
	std::vector<regidx_t> face_regs, vtx_regs;
	// Assignments face/vtx/corner (target) index to attribute index. Each target can refer multiple attribute list up to num_bindings_*.
	std::vector<attridx_t> bindings_face_attr, bindings_vtx_attr, bindings_corner_attr;
	listidx_t num_bindings_face, num_bindings_vtx, num_bindings_corner;
	// Assignments region to attribute list index. First a lookup to the offset lists must be done in order to get the offsets in the bindings array.
	std::vector<listidx_t> bindings_reg_facelist, bindings_reg_vtxlist, bindings_reg_cornerlist;
	std::vector<int> off_reg_facelist, off_reg_vtxlist, off_reg_cornerlist;

	Faces &faces;

	Bindings(Faces &_faces) :
		off_reg_facelist(1, 0), off_reg_vtxlist(1, 0), off_reg_cornerlist(1, 0),
		num_bindings_face(0), num_bindings_vtx(0), num_bindings_corner(0),
		faces(_faces)
	{}

	attridx_t &binding_face_attr(faceidx_t f, listidx_t a)
	{
		return bindings_face_attr[f * num_bindings_face + a];
	}
	attridx_t &binding_vtx_attr(vtxidx_t v, listidx_t a)
	{
		return bindings_vtx_attr[v * num_bindings_vtx + a];
	}
	attridx_t &binding_corner_attr(faceidx_t f, ledgeidx_t v, listidx_t a)
	{
		return bindings_corner_attr[(faces.off(f) + v) * num_bindings_corner + a];
	}

	listidx_t &binding_reg_facelist(regidx_t r, listidx_t a)
	{
		return bindings_reg_facelist[off_reg_facelist[r] + a];
	}
	listidx_t &binding_reg_vtxlist(regidx_t r, listidx_t a)
	{
		return bindings_reg_vtxlist[off_reg_vtxlist[r] + a];
	}
	listidx_t &binding_reg_cornerlist(regidx_t r, listidx_t a)
	{
		return bindings_reg_cornerlist[off_reg_cornerlist[r] + a];
	}

	regidx_t num_regs_face() const
	{
		return off_reg_facelist.size() - 1;
	}
	regidx_t num_regs_vtx() const
	{
		return off_reg_vtxlist.size() - 1;
	}

	// actual number of bindings in a region for a face/vtx/corner
	listidx_t num_bindings_face_reg(regidx_t r) const
	{
		return off_reg_facelist[r + 1] - off_reg_facelist[r];
	}
	listidx_t num_bindings_vtx_reg(regidx_t r) const
	{
		return off_reg_vtxlist[r + 1] - off_reg_vtxlist[r];
	}
	listidx_t num_bindings_corner_reg(regidx_t r) const
	{
		return off_reg_cornerlist[r + 1] - off_reg_cornerlist[r];
	}

	faceidx_t num_face() const
	{
		return face_regs.size();
	}
	vtxidx_t num_vtx() const
	{
		return vtx_regs.size();
	}
	edgeidx_t num_edge() const
	{
		return faces.size_edge();
	}

	regidx_t face2reg(faceidx_t f) const
	{
		return face_regs[f];
	}
	regidx_t vtx2reg(vtxidx_t v) const
	{
		return vtx_regs[v];
	}
};

struct Attrs : std::vector<Attr>, Bindings {
	Attrs(Faces &faces) : Bindings(faces)
	{}
};

}
}
