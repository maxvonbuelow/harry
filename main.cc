/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "../structs/mesh.h"
#include "../structs/quant.h"
#include "../formats/unified_reader.h"
#include "../formats/unified_writer.h"

#include "assert.h"
#include "args.h"

struct Args {
	struct Quant {
		int l, o, q;
	};
	std::string in, out;
	unified::writer::FileType fmt;
	std::vector<Quant> quant;
	bool clearquant;
	bool ply_ascii;

	Args(int argc, char **argv) : fmt(unified::writer::UNKNOWN), ply_ascii(false), quant(false), clearquant(false)
	{
		using namespace std::string_literals;
		args::parser args(argc, argv, "Harry mesh compressor");
		const int ARG_IN  = args.add_nonopt("INPUT");
		const int ARG_OUT = args.add_nonopt("OUTPUT"); args.range(2, 2);
		const int ARG_FMT = args.add_opt('f', "format",      "Enforce output format");
		const int ARG_LST = args.add_opt('l', "list",        "Select attribute list");
		const int ARG_ATT = args.add_opt('a', "attr",        "Select attribute");
		const int ARG_QUA = args.add_opt('q', "quant",       "Quantization bits");
		const int ARG_CQU = args.add_opt('c', "clear-quant", "Clear all quantization first");
		const int ARG_PAS = args.add_opt(     "ply-ascii",   "PLY writer: Use ASCII format");

		int cur_l, cur_a = -1;
		for (int arg = args.next(); arg != args::parser::end; arg = args.next()) {
			if (arg == ARG_IN)       in         = args.val<std::string>();
			else if (arg == ARG_OUT) out        = args.val<std::string>();
			else if (arg == ARG_FMT) fmt        = args.map("hry"s, unified::writer::HRY, "ply"s, unified::writer::PLY, "obj"s, unified::writer::OBJ);
			else if (arg == ARG_LST) cur_l      = args.val<int>();
			else if (arg == ARG_ATT) cur_a      = args.val<int>();
			else if (arg == ARG_QUA) { quant.push_back(Quant{ cur_l, cur_a, args.val<int>() }); cur_a = -1; }
			else if (arg == ARG_CQU) clearquant = true;
			else if (arg == ARG_PAS) ply_ascii  = true;
		}
	}
};

void convert_quant(const mesh::attr::Attrs &attrs, const std::vector<Args::Quant> &src, std::vector<quant::Quant> &dst)
{
	for (int i = 0; i < src.size(); ++i) {
		Args::Quant q = src[i];
		if (q.q < 0) throw std::runtime_error("Invalid quantization bits");
		if (q.l < 0 || q.l >= attrs.size()) throw std::runtime_error("Invalid list index");
		if (q.o == -1) {
			for (int o = 0; o < attrs[q.l].fmt().size(); ++o) {
				if (q.q > attrs[q.l].fmt().bytes(o) * 8) throw std::runtime_error("Invalid quantization bits");
				dst.push_back(quant::Quant(q.l, o, q.q));
			}
		} else {
			if (q.o < 0 || q.o >= attrs[q.l].fmt().size()) throw std::runtime_error("Invalid attribute index");
			if (q.q > attrs[q.l].fmt().bytes(q.o) * 8) throw std::runtime_error("Invalid quantization bits");
			dst.push_back(quant::Quant(q.l, q.o, q.q));
		}
	}
}

int main(int argc, char **argv)
{
	AssertMngr::set([] { std::exit(EXIT_FAILURE); });

	Args args(argc, argv);

	mesh::Mesh mesh;
	std::cout << "Reading input" << std::endl;
	unified::reader::read(args.in, mesh);

	if (!args.quant.empty() || args.clearquant) {
		std::cout << "Quantization" << std::endl;
		std::vector<quant::Quant> quant;
		convert_quant(mesh.attrs, args.quant, quant);
		quant::requant(mesh.attrs, quant, args.clearquant);
	}

	std::cout << "Writing output" << std::endl;
	unified::writer::write(args.out, mesh, args.fmt, args.ply_ascii);
}
