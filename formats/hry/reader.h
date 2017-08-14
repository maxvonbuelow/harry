#pragma once

#include "../../structs/mesh.h"
#include <istream>

namespace hry {
namespace reader {

void read(std::istream &is, mesh::Mesh &mesh);

}
}
