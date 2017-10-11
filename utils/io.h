/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <string>
#include <istream>

namespace util {

inline void skip_ws(std::istream &is)
{
	while (std::isspace(is.peek()) && is.peek() != '\n') is.get();
}
inline void skip_line(std::istream &is)
{
	is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

inline std::istream &getline(std::istream &is, std::string &t)
{
	// source: http://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
	t.clear();

	std::istream::sentry se(is, true);
	std::streambuf *sb = is.rdbuf();

	for (;;) {
		int c = sb->sbumpc();
		switch (c) {
		case '\n':
			return is;
		case '\r':
			if (sb->sgetc() == '\n')
				sb->sbumpc();
			return is;
		case EOF:
			if (t.empty())
				is.setstate(std::ios::eofbit);
			return is;
		default:
			t += (char)c;
		}
	}
}

// WARNING: only use with noskipws
template <typename T>
int line2list(std::istream &is, T *list, int max)
{
	int i;
	util::skip_ws(is);
	for (i = 0; i < max; ++i) {
		if (is.peek() == '\n') { is.get(); break; }
		is >> list[i];
		util::skip_ws(is);
	}
	return i;
}

inline std::streampos linenum_approx(std::istream &is, int samples = 100)
{
	is.seekg(0, std::ios::beg);
	std::streampos beg = is.tellg();
	is.seekg(0, std::ios::end);
	std::streampos end = is.tellg();
	std::streampos diff = end - beg;

	std::streampos sum = 0;
	for (int i = 0; i < samples; ++i) {
		std::streampos off = (diff * i + diff / 2) / samples;
		is.seekg(off, std::ios::beg);
		skip_line(is);
		std::streampos begline = is.tellg();
		skip_line(is);
		std::streampos endline = is.tellg();
		std::streampos linesize = endline - begline;
		sum += linesize;
	}

	is.clear();
	is.seekg(0, std::ios::beg);

	return diff / (sum / samples);
}

inline std::string join_path(const std::string &a, const std::string &b)
{
	return a + std::string("/") + b;
}

}
