#include <iostream>

#include "../structs/mesh.h"
#include "../formats/unified_reader.h"
#include "../formats/unified_writer.h"

#include "assert.h"
#include "args.h"

struct Args {
	std::string in, out;

	Args(int argc, char **argv)
	{
		args::parser args(argc, argv, "Harry mesh compressor");
		const int ARG_IN  = args.add_nonopt("INPUT");
		const int ARG_OUT = args.add_nonopt("OUTPUT"); args.range(2, 2);

		for (int arg = args.next(); arg != args::parser::end; arg = args.next()) {
			if (arg == ARG_IN)       in  = args.val<std::string>();
			else if (arg == ARG_OUT) out = args.val<std::string>();
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
	unified::writer::write(args.out, mesh);
}
