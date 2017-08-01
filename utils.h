#pragma once

#include <unordered_map>
#include <string>
#include <fstream>

#include <sys/stat.h>
#include <sys/types.h>

namespace util {

typedef std::unordered_map<std::string, int> stringmap;

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

inline bool mkdir(const char *path)
{
#ifdef _WIN32
	if (::_mkdir(path) < 0) return false;
#else // _WIN32
	if (::mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP) < 0) return false;
#endif // _WIN32
	return true;
}

enum FileType { FILE, DIR, OTHER, ERROR };
inline FileType filetype(const char *path)
{
	struct stat s;
	if (::stat(path, &s) != 0) return ERROR;
	if (s.st_mode & S_IFREG) return FILE;
	if (s.st_mode & S_IFDIR) return DIR;
	return OTHER;
}
inline bool isfile(const char *path)
{
	return filetype(path) == FILE;
}
inline bool isdir(const char *path)
{
	return filetype(path) == DIR;
}

inline std::streampos linenum_approx(std::istream &is, int quantiles = 100)
{
	is.seekg(0, std::ios::beg);
	std::streampos beg = is.tellg();
	is.seekg(0, std::ios::end);
	std::streampos end = is.tellg();
	std::streampos diff = end - beg;

	std::streampos sum = 0;
	for (int i = 0; i < quantiles; ++i) {
		std::streampos off = (diff * i + diff / 2) / quantiles;
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

	return diff / (sum / quantiles);
}

inline std::string join_path(const std::string &a, const std::string &b)
{
	return a + std::string("/") + b;
}

}
