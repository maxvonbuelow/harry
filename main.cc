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
#include "../formats/unified_reader.h"
#include "../formats/unified_writer.h"

#include "assert.h"
#include "args.h"

struct Args {
	std::string in, out;
	unified::writer::FileType fmt;
	bool ply_ascii;

	Args(int argc, char **argv) : fmt(unified::writer::UNKNOWN), ply_ascii(false)
	{
		using namespace std::string_literals;
		args::parser args(argc, argv, "Harry mesh compressor");
		const int ARG_IN  = args.add_nonopt("INPUT");
		const int ARG_OUT = args.add_nonopt("OUTPUT"); args.range(2, 2);
		const int ARG_FMT = args.add_opt('f', "format",    "Enforce output format");
		const int ARG_PAS = args.add_opt(     "ply-ascii", "PLY writer: Use ASCII format.");

		for (int arg = args.next(); arg != args::parser::end; arg = args.next()) {
			if (arg == ARG_IN)       in  = args.val<std::string>();
			else if (arg == ARG_OUT) out = args.val<std::string>();
			else if (arg == ARG_FMT) fmt = args.map("hry"s, unified::writer::HRY, "ply"s, unified::writer::PLY, "obj"s, unified::writer::OBJ);
			else if (arg == ARG_PAS) ply_ascii = true;
		}
	}
};

int main(int argc, char **argv)
{
	AssertMngr::set([] { std::exit(EXIT_FAILURE); });

	Args args(argc, argv);

	mesh::Mesh mesh;
	std::cout << "Reading input" << std::endl;
	unified::reader::read(args.in, mesh);
	std::cout << "Writing output" << std::endl;
	unified::writer::write(args.out, mesh, args.fmt, args.ply_ascii);
}
