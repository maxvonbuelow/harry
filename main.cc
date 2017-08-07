#include <iostream>
#include "../structs/mesh.h"
#include "../formats/unified_reader.h"
#include "../formats/unified_writer.h"
#include <fstream>

// #include "../assert.cc"

int main(int argc, char **argv)
{
	mesh::Mesh mesh;
	mesh::Builder builder(mesh);
	unified::reader::read<mesh::Builder>(argv[1], builder);
	unified::writer::write<mesh::Mesh>(argv[2], mesh);
}
