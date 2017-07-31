#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <numeric>
#include <algorithm>
#include "../structs/mixing.h"
#include "../utils.h"

namespace reader {
namespace ply {

inline float uint2float(uint32_t i)
{
	return *((float*)&i);
}
inline double uint2double(uint64_t i)
{
	return *((double*)&i);
}
template <typename T>
T is_read_type(std::istream &is) {
	T val;
	is.read((char*)&val, sizeof(T));
	return val;
}

enum Fmt { ASCII, BIN_LE, BIN_BE };
typedef mixing::Type Type;

enum WellKnownProps {
	PX, PY, PZ, PW,
	NX, NY, NZ, NW,
	CR, CG, CB,
	CAR, CAG, CAB, CAC,
	CDR, CDG, CDB, CDC,
	CSR, CSG, CSB, CSP, CSC,
	TU, TV, TW,
	SCALE, CONFIDENCE, WKEND
};

static std::unordered_map<std::string, int> lut_well_known = {
	{ "x", PX }, { "y", PY }, { "z", PZ }, { "w", PW },
	{ "nx", NX }, { "ny", NY }, { "nz", NZ }, { "nw", NW },
	{ "red", CR }, { "green", CG }, { "blue", CB },
	{ "ambient_red", CAR }, { "ambient_green", CAG }, { "ambient_blue", CAB }, { "ambient_coeff", CAC },
	{ "diffuse_red", CDR }, { "diffuse_green", CDG }, { "diffuse_blue", CDB }, { "diffuse_coeff", CDC },
	{ "specular_red", CSR }, { "specular_green", CSG }, { "specular_blue", CSB }, { "specular_power", CSP }, { "specular_coeff", CSC },
	{ "u", TU }, { "tu", TU }, { "v", TV }, { "tv", TV }, { "tw", TW },
	{ "value", SCALE }, { "scale", SCALE }, { "confidence", CONFIDENCE },
};
static const mixing::Interp lut_interp[/*27*/] = {
	mixing::POS, mixing::POS, mixing::POS, mixing::POS,
	mixing::NORMAL, mixing::NORMAL, mixing::NORMAL, mixing::NORMAL,
	mixing::COLOR, mixing::COLOR, mixing::COLOR,
	mixing::COLOR_AMBIENT, mixing::COLOR_AMBIENT, mixing::COLOR_AMBIENT, mixing::COLOR_AMBIENT,
	mixing::COLOR_DIFFUSE, mixing::COLOR_DIFFUSE, mixing::COLOR_DIFFUSE, mixing::COLOR_DIFFUSE,
	mixing::COLOR_SPECULAR, mixing::COLOR_SPECULAR, mixing::COLOR_SPECULAR, mixing::COLOR_SPECULAR, mixing::COLOR_SPECULAR,
	mixing::TEX, mixing::TEX, mixing::TEX,
	mixing::SCALE, mixing::CONFIDENCE,
};

inline Type str2type(const std::string &s)
{
	if (s == "float"  || s == "float32") return mixing::FLOAT;
	if (s == "double" || s == "float64") return mixing::DOUBLE;
	if (s == "uint"   || s == "uint32")  return mixing::UINT;
	if (s == "int"    || s == "int32")   return mixing::INT;
	if (s == "ushort" || s == "uint16")  return mixing::USHORT;
	if (s == "short"  || s == "int16")   return mixing::SHORT;
	if (s == "uchar"  || s == "uint8")   return mixing::UCHAR;
	if (s == "char"   || s == "int8")    return mixing::CHAR;
	throw std::runtime_error("Invalid data type");
};

struct Property {
	std::string name;
	Type type;
	Type list_len_type;

	inline Property(const std::string &_name, Type _type, Type _list_len_type = mixing::NONE) : name(_name), type(_type), list_len_type(_list_len_type)
	{}
};
struct Element : std::vector<Property> {
	std::string name;
	int len;
	std::unordered_map<std::string, int> propmap;

	inline Element(const std::string &_name, int _len) : name(_name), len(_len)
	{}

	inline int add(const std::string &name, Type type, Type list_len_type = mixing::NONE)
	{
		int idx = this->size();
		propmap[name] = idx;
		this->push_back(Property(name, type, list_len_type));
		return idx;
	}
	inline int operator[](const std::string &name) const
	{
		std::unordered_map<std::string, int>::const_iterator it = propmap.find(name);
		return it == propmap.end() ? -1 : it->second;
	}
	inline const Property &operator[](int i) const
	{
		return std::vector<Property>::operator[](i);
	}
	inline Property &operator[](int i)
	{
		return std::vector<Property>::operator[](i);
	}

	void init(std::vector<int> &perm, mixing::Fmt &fmt, mixing::Interps &interps) const
	{
		// perm: property -> attribute offset
		// invperm: attribute offset -> property

		// set weights
		int otherweight = WKEND;
		std::vector<int> weights(this->size(), std::numeric_limits<int>::max());
		int valid = 0;
		for (int i = 0; i < this->size(); ++i) {
			const Property &prop = (*this)[i];
			std::unordered_map<std::string, int>::const_iterator it = lut_well_known.find(prop.name);
			if (prop.list_len_type != mixing::NONE) continue;
			int weight;
			if (it == lut_well_known.end()) weight = otherweight++;
			else weight = it->second;
			weights[i] = weight;
			++valid;
		}

		// generate inverse permutation
		std::vector<int> invperm(weights.size());
		std::iota(invperm.begin(), invperm.end(), 0);
		std::sort(invperm.begin(), invperm.end(), [&weights](std::size_t i, std::size_t j) { return weights[i] < weights[j]; });

		// generate permutation and attach type and interpretation
		perm.resize(weights.size(), -1);
		int oid = 0;
		for (int i = 0; i < valid; ++i) {
			int target = invperm[i];
			fmt.add((*this)[target].type);
			int weight = weights[target];
			int interp = weight >= WKEND ? mixing::OTHER + oid++ : lut_interp[weight];
			interps.append(interp, i);
			if (interp >= mixing::OTHER) interps.describe(interp, (*this)[target].name);

			perm[target] = i;
		}
	}
};
struct Header : std::vector<Element> {
	Fmt fmt;
	std::unordered_map<std::string, int> elemmap;

	inline int add(const std::string &name, int len)
	{
		int idx = this->size();
		elemmap[name] = idx;
		this->push_back(Element(name, len));
		return idx;
	}

	inline int operator[](const std::string &name) const
	{
		std::unordered_map<std::string, int>::const_iterator it = elemmap.find(name);
		return it == elemmap.end() ? -1 : it->second;
	}
	inline const Element &operator[](int i) const
	{
		return std::vector<Element>::operator[](i);
	}
	inline Element &operator[](int i)
	{
		return std::vector<Element>::operator[](i);
	}
};

Header read_header(std::istream &is)
{
	std::string id;
	Header header;
	int curelem = -1;

	do {
		is >> id;
		if (id == "ply") {
			// #whatever
		} else if (id == "format") {
			std::string fmt;
			is >> fmt;
			if (fmt == "ascii") header.fmt = ASCII;
			else if (fmt == "binary_little_endian") header.fmt = BIN_LE;
			else if (fmt == "binary_big_endian") header.fmt = BIN_BE;
			else throw std::runtime_error("Invlaid format");
			util::skip_line(is);
		} else if (id == "comment") {
			util::skip_line(is);
		} else if (id == "element") {
			std::string name;
			is >> name;
			int len;
			is >> len;
			curelem = header.add(name, len);
		} else if (id == "property") {
			if (curelem == -1) throw std::runtime_error("Invlaid property");
			std::string type, name;
			mixing::Type list_len_type = mixing::NONE;

			is >> type;
			if (type == "list") {
				is >> type;
				list_len_type = str2type(type);
				is >> type;
			}
			is >> name;

			header[curelem].add(name, str2type(type), list_len_type);
		} else if (id != "end_header") {
			std::cout << "Skipping invalid PLY line " << id << std::endl;
			util::skip_line(is);
		}
	} while (id != "end_header");
	util::skip_line(is);

	return header;
}

template <typename T>
T cpval2ptr(unsigned char *ptr, T val)
{
	std::copy(&val, &val + sizeof(T), ptr);
	return val;
}
struct ASCIIReader {
	uint64_t operator()(std::istream &is, unsigned char *dst, Type type)
	{
		int64_t val;
		uint64_t uval;
		double fval;
		switch (type) {
		case mixing::NONE:
			return 1;
		case mixing::CHAR:
			is >> val;
			return cpval2ptr<int8_t>(dst, val);
		case mixing::UCHAR:
			is >> uval;
			return cpval2ptr<uint8_t>(dst, uval);
		case mixing::SHORT:
			is >> val;
			return cpval2ptr<int16_t>(dst, val);
		case mixing::USHORT:
			is >> uval;
			return cpval2ptr<uint16_t>(dst, uval);
		case mixing::INT:
			is >> val;
			return cpval2ptr<int32_t>(dst, val);
		case mixing::UINT:
			is >> uval;
			return cpval2ptr<uint32_t>(dst, uval);
		case mixing::FLOAT:
			is >> fval;
			return cpval2ptr<float>(dst, fval);
		case mixing::DOUBLE:
			is >> fval;
			return cpval2ptr<double>(dst, fval);
		}
	}
};
struct BinBEReader {
	uint64_t operator()(std::istream &is, unsigned char *dst, Type type)
	{
		switch (type) {
		case mixing::NONE:   return 1;
		case mixing::CHAR:   return cpval2ptr<int8_t>  (dst,                     is_read_type<int8_t>(is));
		case mixing::UCHAR:  return cpval2ptr<uint8_t> (dst,                     is_read_type<uint8_t>(is));
		case mixing::SHORT:  return cpval2ptr<int16_t> (dst,             be16toh(is_read_type<int16_t>(is)));
		case mixing::USHORT: return cpval2ptr<uint16_t>(dst,             be16toh(is_read_type<uint16_t>(is)));
		case mixing::INT:    return cpval2ptr<int32_t> (dst,             be32toh(is_read_type<int32_t>(is)));
		case mixing::UINT:   return cpval2ptr<uint32_t>(dst,             be32toh(is_read_type<uint32_t>(is)));
		case mixing::FLOAT:  return cpval2ptr<float>   (dst, uint2float( be32toh(is_read_type<uint32_t>(is))));
		case mixing::DOUBLE: return cpval2ptr<double>  (dst, uint2double(be64toh(is_read_type<uint64_t>(is))));
		}
	}
};
struct BinLEReader {
	uint64_t operator()(std::istream &is, unsigned char *dst, Type type)
	{
		switch (type) {
		case mixing::NONE:   return 1;
		case mixing::CHAR:   return cpval2ptr<int8_t>  (dst,                     is_read_type<int8_t>(is));
		case mixing::UCHAR:  return cpval2ptr<uint8_t> (dst,                     is_read_type<uint8_t>(is));
		case mixing::SHORT:  return cpval2ptr<int16_t> (dst,             le16toh(is_read_type<int16_t>(is)));
		case mixing::USHORT: return cpval2ptr<uint16_t>(dst,             le16toh(is_read_type<uint16_t>(is)));
		case mixing::INT:    return cpval2ptr<int32_t> (dst,             le32toh(is_read_type<int32_t>(is)));
		case mixing::UINT:   return cpval2ptr<uint32_t>(dst,             le32toh(is_read_type<uint32_t>(is)));
		case mixing::FLOAT:  return cpval2ptr<float>   (dst, uint2float( le32toh(is_read_type<uint32_t>(is))));
		case mixing::DOUBLE: return cpval2ptr<double>  (dst, uint2double(le64toh(is_read_type<uint64_t>(is))));
		}
	}
};

template <typename T, typename R>
void readloop(std::istream &is, const Header &header, const std::vector<int> *perms, T &handle, R &&read)
{
	int face_idx = header["face"];
	int vtx_idx = header["vertex"];
	int vi_idx = header[face_idx]["vertex_indices"];
	for (int i = 0; i < header.size(); ++i) {
		const Element &elem = header[i];
		unsigned char ign[8];

		if (i == face_idx || i == vtx_idx) {
			int list = i == face_idx ? 0 : 1;
			const std::vector<int> &perm = perms[list];

			for (int j = 0; j < elem.len; ++j) {
				for (int k = 0; k < elem.size(); ++k) {
					const Property &prop = elem[k];
					if (perm[j] == -1) { // not an attribute
						uint64_t listlen = read(is, ign, prop.list_len_type);
						if (prop.list_len_type != mixing::NONE && i == face_idx && k == vi_idx) { // ...but connectivity
							handle.face_begin(listlen);
							for (int l = 0; l < listlen; ++l) {
								handle.set_org(read(is, ign, prop.type));
							}
							handle.face_end();
						} else { // no connectivity and not an attribute: ignore
							for (int l = 0; l < listlen; ++l) {
								read(is, ign, prop.type);
							}
						}
					} else { // attribute
						unsigned char *dst = handle.elem(list, j, perm[k]);
						if (dst != NULL) read(is, dst, prop.type);
					}
				}
			}
		} else {
			// ignore unknown elements
			for (int j = 0; j < elem.len; ++j) {
				for (int k = 0; k < elem.size(); ++k) {
					const Property &prop = elem[k];
					uint64_t listlen = read(is, ign, prop.list_len_type);
					for (int l = 0; l < listlen; ++l) {
						read(is, ign, prop.type);
					}
				}
			}
		}
	}
}

template <typename H>
void read(std::istream &is, H &handle)
{
	Header header = read_header(is);

	// add attributes and allocate memory
	int idxs[] = { header["face"], header["vertex"] };

	std::vector<int> perms[2];
	for (int i = 0; i < 2; ++i) {
		mixing::Fmt fmt;
		mixing::Interps interps;
		header[idxs[i]].init(perms[i], fmt, interps);
		handle.add_list(fmt, interps);
		handle.alloc_attr(i, header[idxs[i]].len);
	}
	handle.bind_face_list(handle.add_face_region(1), 0, 0);
	handle.bind_vtx_list(handle.add_vtx_region(1), 0, 1);

	int nf = header[idxs[0]].len, nv = header[idxs[1]].len;
	handle.alloc_face(nf);
	handle.alloc_vtx(nv);
	for (int i = 0; i < nf; ++i) {
		handle.bind_face_attr(i, 0, i);
	}
	for (int i = 0; i < nv; ++i) {
		handle.bind_vtx_attr(i, 0, i);
	}

	// load mesh
	switch (header.fmt)
	{
	case ASCII:
		readloop(is, header, perms, handle, ASCIIReader());
		break;
	case BIN_BE:
		readloop(is, header, perms, handle, BinBEReader());
		break;
	case BIN_LE:
		readloop(is, header, perms, handle, BinLEReader());
		break;
	}
}

}
}
