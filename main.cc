#include <iostream>
#include "../structs/mesh.h"
#include "../formats/unified_reader.h"
#include "../formats/unified_writer.h"

int main(int argc, char **argv)
{
	mesh::Mesh mesh;
	mesh::Builder builder(mesh);
	std::cout << "Reading input" << std::endl;
	unified::reader::read<mesh::Builder>(argv[1], builder);
	std::cout << "Writing output" << std::endl;
	unified::writer::write<mesh::Mesh>(argv[2], mesh);
}
