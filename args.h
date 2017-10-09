/* 
 * Argument parser
 * 
 * Copyright (c) 2017, Max von Buelow
 * All rights reserved.
 * Contact: https://maxvonbuelow.de
 * 
 * This file is part of the HOTools project.
 * https://github.com/magcks/hotools
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <unordered_map>
#include <string>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <limits>

namespace args {

// Don't use type_traits for c++11 support
template <typename T, typename U>
struct Conv { T operator()(U txt) { return U(txt); } };
#define FN_CONV(TYPE, CONV) template <typename U> struct Conv<TYPE, U> { TYPE operator()(U txt) { return CONV(txt); } };
FN_CONV(float, std::atof); FN_CONV(double, std::atof);
FN_CONV(int8_t, std::atoi); FN_CONV(uint8_t, std::atoi);
FN_CONV(int16_t, std::atoi); FN_CONV(uint16_t, std::atoi);
FN_CONV(int32_t, std::atoi); FN_CONV(uint32_t, std::atoi);
FN_CONV(int64_t, std::atoi); FN_CONV(uint64_t, std::atoi);

struct parser {
	static const int end = -2;
	static const int invalid = -1;

	std::string description;

	enum Type { OPT, LOPT, VAL };
private:
	struct Opt {
		char opt;
		std::string lopt;
		std::string descr;
	};
	enum Reason { E_CNT_LT, E_CNT_GT, E_INVALID, E_INVALID_LOPT, E_CAST, E_ENUM };
	int argc;
	char **argv;
	int cur;
	char *curopt;
	int nonopt_cur;
	int nonopt_min, nonopt_max;
	int opts[256];
	std::vector<int> nonopts;
	int id;
	std::unordered_map<std::string, int> lopts;
	std::vector<Opt> optinfo;
	std::vector<std::string> nonoptinfo;
	int maxw;
	int optid_help;
	bool val_read;
	std::ostream &os;

public:
	parser(int _argc, char **_argv, const std::string &_description = "", std::ostream &_os = std::cout) : argc(_argc), argv(_argv), cur(1), id(0), curopt(argv[cur]), nonopt_cur(0), maxw(0), description(_description), val_read(true), os(_os)
	{
		for (int i = 0; i < 256; ++i) opts[i] = invalid;
		range(0);
		optid_help = add_opt('h', "help", "Print this dialogue.");
	}

	void range(int min, int max) // for non-optionals
	{
		nonopt_min = min;
		nonopt_max = max;
	}
	void range(int min)
	{
		range(min, std::numeric_limits<int>::max());
	}

	int add_opt(char opt, const std::string &lopt, const std::string &descr)
	{
		opts[opt] = id;
		lopts[lopt] = id;
		optinfo.push_back(Opt{ opt, lopt, descr });
		maxw = std::max(maxw, (int)lopt.size());
		return id++;
	}
	int add_opt(const std::string &lopt, const std::string &descr)
	{
		return add_opt('\0', lopt, descr);
	}
	int add_nonopt(const std::string &name)
	{
		nonoptinfo.push_back(name);
		nonopts.push_back(id);
		return id++;
	}

	void show_usage()
	{
		if (!description.empty()) os << description << std::endl << std::endl;
		os << "Usage: " << argv[0] << " [OPTIONS]";
		for (int i = 0; i < nonoptinfo.size(); ++i) {
			if (i >= nonopt_min)
				os << " [" << nonoptinfo[i] << "]";
			else
				os << " " << nonoptinfo[i];
		}
		if (nonopt_max > nonopts.size()) os << " ...";
		os << std::endl;
		for (const Opt &opt : optinfo) {
			os << "  ";
			if (opt.opt) os << '-' << opt.opt << ", ";
			else os << "    ";
			os << "--" << std::left << std::setw(maxw + 2) << opt.lopt << opt.descr << std::endl;
		}
	}

	Type nexttype()
	{
		return *curopt == '-' && curopt[1] && curopt[1] != '-' ? OPT :
		       *curopt == '-' && curopt[1] == '-' && curopt[2] ? LOPT :
		       VAL;
	}

	bool has_val()
	{
		return nexttype() == VAL;
	}

	int next()
	{
		if (cur >= argc) {
			if (nonopt_cur < nonopt_min) err(E_CNT_LT);
			return end;
		}
		Type type = nexttype();

		int i;
		if (type == LOPT) {
			curopt += 2;
			int len;
			for (len = 0; curopt[len] && curopt[len] != '='; ++len);
			std::string lopt(curopt, len);
			curopt += len + (curopt[len] == '=' ? 1 : 0);
			i = lopts.find(lopt) != lopts.end() ? lopts[lopt] : invalid;
			if (i == invalid) err(E_INVALID_LOPT);
			if (!*curopt) inc();
		} else if (type == OPT) {
			++curopt;
			i = opts[*curopt];
			if (i == invalid) err(E_INVALID);
			++curopt;
			val_read = false;
			if (!*curopt) inc();
		} else if (!val_read) {
			// last value was not read: assume boolean -> no need for additional dash
			i = opts[*curopt];
			if (i == invalid) err(E_INVALID);
			++curopt;
			if (!*curopt) inc();
		} else {
			if (nonopt_cur >= nonopt_max) err(E_CNT_GT);
			i = nonopt_cur >= nonopts.size() ? nonopts.back() : nonopts[nonopt_cur++];
		}

		if (i == optid_help) {
			show_usage();
			std::exit(0);
			i = next();
		}
		return i;
	}
	template <typename T>
	T val()
	{
		val_read = true;
		T v;
		try {
			if (!curopt) err(E_CNT_LT);
			else v = Conv<T, char*>()(curopt);
		} catch (...) {
			err(E_CAST);
			return T();
		}
		inc();
		return v;
	}

	template <typename TK, typename TV, typename ...T>
	TV &&map(TK &&key, TV &&mapped, T &&...args)
	{
		return std::move(_map(std::move(val<typename std::remove_reference<TK>::type>()), std::move(key), std::move(mapped), std::move(args)...));
	}

private:
	template <typename C, typename TK, typename TV, typename ...T>
	TV _map(C &&val, TK &&key, TV &&mapped)
	{
		if (key == val) return std::move(mapped);
		else err(E_ENUM);
	}

	template <typename C, typename TK, typename TV, typename ...T>
	TV &&_map(C &&val, TK &&key, TV &&mapped, T &&...args)
	{
		if (key == val) return std::move(mapped);
		else return std::move(_map<C, T...>(std::move(val), std::move(args)...));
	}

	void err(Reason reason)
	{
		show_usage();
		os << std::endl;
		switch (reason) {
		case E_CNT_LT:
			std::cerr << "Error: Too few non-optional arguments" << std::endl;
			break;
		case E_CNT_GT:
			std::cerr << "Error: Too much non-optional arguments" << std::endl;
			break;
		case E_INVALID:
			std::cerr << "Error: Invalid option " << *curopt << std::endl;
			break;
		case E_INVALID_LOPT:
			std::cerr << "Error: Invalid option " << curopt << std::endl;
			break;
		case E_CAST:
			std::cerr << "Error: Invalid cast" << std::endl;
			break;
		case E_ENUM:
			std::cerr << "Error: Invalid enum value" << std::endl;
			break;
		}
		std::exit(1);
	}

	void inc()
	{
		++cur;
		curopt = argv[cur];
		val_read = true;
	}
};

}
