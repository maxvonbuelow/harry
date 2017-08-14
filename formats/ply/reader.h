#pragma once

#include <istream>
#include "../../structs/mesh.h"

namespace ply {
namespace reader {

void read(std::istream &is, mesh::Mesh &mesh);

}
}
