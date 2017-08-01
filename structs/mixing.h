#pragma once

#include <vector>
#include <stdint.h>

namespace mixing {

static const int SIZES[11] = { 4, 8, 8, 8, 4, 4, 2, 2, 1, 1, 0 };
enum Type { FLOAT, DOUBLE, ULONG, LONG, UINT, INT, USHORT, SHORT, UCHAR, CHAR, NONE };
enum Interp {
	POS, NORMAL,
	COLOR, COLOR_AMBIENT, COLOR_DIFFUSE, COLOR_SPECULAR, COLOR_TRANSFILTER,
	COLOR_AMBIENT_XYZ, COLOR_DIFFUSE_XYZ, COLOR_SPECULAR_XYZ, COLOR_TRANSFILTER_XYZ,
	COLOR_SPECULAR_EXP, SHARPNESS, DENSITY,
	TRANSPARENCY, TRANSPARENCY_HALO,
	TEX, SCALE, CONFIDENCE,

	OTHER,
	TEX_MAP_ANTIALIAS,
	TEX_MAP_AMBIENT_ID, TEX_MAP_AMBIENT_BLENDU, TEX_MAP_AMBIENT_BLENDV, TEX_MAP_AMBIENT_COLORCORRECT, TEX_MAP_AMBIENT_CLAMP, TEX_MAP_AMBIENT_RANGE, TEX_MAP_AMBIENT_O, TEX_MAP_AMBIENT_S, TEX_MAP_AMBIENT_T, TEX_MAP_AMBIENT_RES,
	TEX_MAP_DIFFUSE_ID, TEX_MAP_DIFFUSE_BLENDU, TEX_MAP_DIFFUSE_BLENDV, TEX_MAP_DIFFUSE_COLORCORRECT, TEX_MAP_DIFFUSE_CLAMP, TEX_MAP_DIFFUSE_RANGE, TEX_MAP_DIFFUSE_O, TEX_MAP_DIFFUSE_S, TEX_MAP_DIFFUSE_T, TEX_MAP_DIFFUSE_RES,
	TEX_MAP_SPECULAR_ID, TEX_MAP_SPECULAR_BLENDU, TEX_MAP_SPECULAR_BLENDV, TEX_MAP_SPECULAR_COLORCORRECT, TEX_MAP_SPECULAR_CLAMP, TEX_MAP_SPECULAR_RANGE, TEX_MAP_SPECULAR_O, TEX_MAP_SPECULAR_S, TEX_MAP_SPECULAR_T, TEX_MAP_SPECULAR_RES,
	TEX_MAP_EXP_ID, TEX_MAP_EXP_BLENDU, TEX_MAP_EXP_BLENDV, TEX_MAP_EXP_COLORCORRECT, TEX_MAP_EXP_CLAMP, TEX_MAP_EXP_CHAN, TEX_MAP_EXP_RANGE, TEX_MAP_EXP_O, TEX_MAP_EXP_S, TEX_MAP_EXP_T, TEX_MAP_EXP_RES,
	TEX_MAP_DISSOLVE_ID, TEX_MAP_DISSOLVE_BLENDU, TEX_MAP_DISSOLVE_BLENDV, TEX_MAP_DISSOLVE_COLORCORRECT, TEX_MAP_DISSOLVE_CLAMP, TEX_MAP_DISSOLVE_CHAN, TEX_MAP_DISSOLVE_RANGE, TEX_MAP_DISSOLVE_O, TEX_MAP_DISSOLVE_S, TEX_MAP_DISSOLVE_T, TEX_MAP_DISSOLVE_RES,
	TEX_MAP_DECAL_ID, TEX_MAP_DECAL_BLENDU, TEX_MAP_DECAL_BLENDV, TEX_MAP_DECAL_COLORCORRECT, TEX_MAP_DECAL_CLAMP, TEX_MAP_DECAL_CHAN, TEX_MAP_DECAL_RANGE, TEX_MAP_DECAL_O, TEX_MAP_DECAL_S, TEX_MAP_DECAL_T, TEX_MAP_DECAL_RES,
	TEX_MAP_DISP_ID, TEX_MAP_DISP_BLENDU, TEX_MAP_DISP_BLENDV, TEX_MAP_DISP_COLORCORRECT, TEX_MAP_DISP_CLAMP, TEX_MAP_DISP_CHAN, TEX_MAP_DISP_RANGE, TEX_MAP_DISP_O, TEX_MAP_DISP_S, TEX_MAP_DISP_T, TEX_MAP_DISP_RES,
	TEX_MAP_BUMP_ID, TEX_MAP_BUMP_MUL, TEX_MAP_BUMP_BLENDU, TEX_MAP_BUMP_BLENDV, TEX_MAP_BUMP_COLORCORRECT, TEX_MAP_BUMP_CLAMP, TEX_MAP_BUMP_CHAN, TEX_MAP_BUMP_RANGE, TEX_MAP_BUMP_O, TEX_MAP_BUMP_S, TEX_MAP_BUMP_T, TEX_MAP_BUMP_RES,
	ILLUM_MODEL,
	/*OTHER*/ };

struct Fmt {
private:
	std::vector<Type> types;
	std::vector<int> quants;

	// derived data
	std::vector<Type> stypes;
	std::vector<int> offsets;

public:
	Fmt() : offsets(1, 0)
	{}

	void add(Type t, int q = 0)
	{
		types.push_back(t);
		quants.push_back(q);
		Type st = q == 0 ? t : quant_type(q);
		stypes.push_back(st);
		offsets.push_back(offsets.back() + SIZES[st]);
	}

	bool isquant(int i) const
	{
		return quants[i] != 0;
	}
	Type stype(int i) const
	{
		return stypes[i];
	}
	Type type(int i) const
	{
		return types[i];
	}
	int offset(int i) const
	{
		return offsets[i];
	}
	int quant(int i) const
	{
		return quants[i];
	}
	int bytes() const
	{
		return offsets.back();
	}
	int size() const
	{
		return types.size();
	}

	static Type quant_type(int q)
	{
		if (q <= 8)  return UCHAR;
		if (q <= 16) return USHORT;
		if (q <= 32) return UINT;
		if (q <= 64) return ULONG;
		return NONE;
	}
};

struct Interps {
	std::vector<int> offs;
	std::vector<int> lens;
	std::vector<std::string> names;

	Interps()
	{}

	void describe(int interp, const std::string &name)
	{
		int nameid = interp - OTHER;
		if (nameid < 0) return;
		if (nameid >= names.size()) names.resize(nameid + 1);
		names[nameid] = name;
	}

	void append(int interp, int off)
	{
		if (interp >= offs.size()) {
			offs.resize(interp + 1, -1);
			lens.resize(interp + 1, 0);
		}

		if (offs[interp] == -1) offs[interp] = off;
		++lens[interp];
	}
};

struct View {
	const Fmt &fmt;
	unsigned char *ptr;

	View(unsigned char *_ptr, const Fmt &_fmt) : ptr(_ptr), fmt(_fmt)
	{}

	unsigned char *data()
	{
		return ptr;
	}
	const unsigned char *data() const
	{
		return ptr;
	}
	int bytes() const
	{
		return fmt.bytes();
	}
	int bytes(int i) const
	{
		return SIZES[fmt.type(i)];
	}
	unsigned char *data(int i)
	{
		return data() + fmt.offset(i);
	}
	const unsigned char *data(int i) const
	{
		return data() + fmt.offset(i);
	}

	template <typename T>
	T &at(int i)
	{
		return *((T*)data(i));
	}
	template <typename T>
	T at(int i) const
	{
		return *((T*)data(i));
	}

	template <typename T>
	T get(int i) const
	{
		switch (fmt.stype(i)) {
		case CHAR:
			return at<int8_t>(i);
		case UCHAR:
			return at<uint8_t>(i);
		case SHORT:
			return at<int16_t>(i);
		case USHORT:
			return at<uint16_t>(i);
		case INT:
			return at<int32_t>(i);
		case UINT:
			return at<uint32_t>(i);
		case LONG:
			return at<int64_t>(i);
		case ULONG:
			return at<uint64_t>(i);
		case FLOAT:
			return at<float>(i);
		case DOUBLE:
			return at<double>(i);
		}
	}

	template <typename T, typename ...Args>
	void set(T &&op, Args ...args)
	{
		for (int i = 0; i < fmt.size(); ++i) {
			switch (fmt.type(i)) {
			case FLOAT:  at<float>(i)    = op.template operator()<float>   (args.template at<float>   (i)...); break;
			case DOUBLE: at<double>(i)   = op.template operator()<double>  (args.template at<double>  (i)...); break;
			case ULONG:  at<uint64_t>(i) = op.template operator()<uint64_t>(args.template at<uint64_t>(i)...); break;
			case LONG:   at<int64_t>(i)  = op.template operator()<int64_t> (args.template at<int64_t> (i)...); break;
			case UINT:   at<uint32_t>(i) = op.template operator()<uint32_t>(args.template at<uint32_t>(i)...); break;
			case INT:    at<int32_t>(i)  = op.template operator()<int32_t> (args.template at<int32_t> (i)...); break;
			case USHORT: at<uint16_t>(i) = op.template operator()<uint16_t>(args.template at<uint16_t>(i)...); break;
			case SHORT:  at<int16_t>(i)  = op.template operator()<int16_t> (args.template at<int16_t> (i)...); break;
			case UCHAR:  at<uint8_t>(i)  = op.template operator()<uint8_t> (args.template at<uint8_t> (i)...); break;
			case CHAR:   at<int8_t>(i)   = op.template operator()<int8_t>  (args.template at<int8_t>  (i)...); break;
			}
		}
	}

	template <typename T, typename ...Args>
	void sets(T &&op, Args ...args)
	{
		for (int i = 0; i < fmt.size(); ++i) {
			switch (fmt.type(i)) {
			case FLOAT:  at<float>(i)    = op.template operator()<float>   (args.template get<float>   (i)...); break;
			case DOUBLE: at<double>(i)   = op.template operator()<double>  (args.template get<double>  (i)...); break;
			case ULONG:  at<uint64_t>(i) = op.template operator()<uint64_t>(args.template get<uint64_t>(i)...); break;
			case LONG:   at<int64_t>(i)  = op.template operator()<int64_t> (args.template get<int64_t> (i)...); break;
			case UINT:   at<uint32_t>(i) = op.template operator()<uint32_t>(args.template get<uint32_t>(i)...); break;
			case INT:    at<int32_t>(i)  = op.template operator()<int32_t> (args.template get<int32_t> (i)...); break;
			case USHORT: at<uint16_t>(i) = op.template operator()<uint16_t>(args.template get<uint16_t>(i)...); break;
			case SHORT:  at<int16_t>(i)  = op.template operator()<int16_t> (args.template get<int16_t> (i)...); break;
			case UCHAR:  at<uint8_t>(i)  = op.template operator()<uint8_t> (args.template get<uint8_t> (i)...); break;
			case CHAR:   at<int8_t>(i)   = op.template operator()<int8_t>  (args.template get<int8_t>  (i)...); break;
			}
		}
	}

	void print(std::ostream &os, int i)
	{
		switch (fmt.type(i)) {
		case CHAR:
		case SHORT:
		case INT:
			os << get<int32_t>(i);
			break;
		case UCHAR:
		case USHORT:
		case UINT:
			os << get<uint32_t>(i);
			break;
		case FLOAT:
		case DOUBLE:
			os << get<double>(i);
			break;
		}
	}
	void print(std::ostream &os, bool append = false, const char *delim = "\t")
	{
		for (int i = 0; i < fmt.size(); ++i) {
			if (i || append) os << delim;
			print(os, i);
		}
	}
};

struct Array {
private:
	std::vector<unsigned char> mdata;
	Fmt mfmt;
	std::size_t msize;

public:
	Array(const Fmt &_fmt) : mfmt(_fmt), msize(0)
	{}

	const Fmt &fmt()
	{
		return mfmt;
	}

	unsigned char *data()
	{
		return mdata.data();
	}
	unsigned int bytes() const
	{
		return mdata.size();
	}
	std::size_t size() const
	{
		return msize;
	}
	void resize(std::size_t size)
	{
		msize = size;
		mdata.resize(size * mfmt.bytes());
		mdata.shrink_to_fit();
	}
	std::size_t frontidx()
	{
		return 0;
	}
	std::size_t backidx()
	{
		return size() ? size() - 1 : 0;
	}
	View front()
	{
		return (*this)[frontidx()];
	}
	View back()
	{
		return (*this)[backidx()];
	}
	View add()
	{
		resize(size() + 1);
		return back();
	}
	View operator[](std::size_t i)
	{
		return View(data() + i * mfmt.bytes(), mfmt);
	}
};

}
