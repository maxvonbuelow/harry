#pragma once

#include <istream>
#include "../../structs/mesh.h"

namespace obj {
namespace reader {

void read(std::istream &is, const std::string &dir, mesh::Mesh &mesh);

}
}
