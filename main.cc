#include <iostream>
#include "../structs/mesh.h"
#include "../formats/unified_reader.h"
#include "../formats/unified_writer.h"

int main(int argc, char **argv)
{
	mesh::Mesh mesh;
	std::cout << "Reading input" << std::endl;
	unified::reader::read(argv[1], mesh);
	std::cout << "Writing output" << std::endl;
	unified::writer::write(argv[2], mesh);
}
