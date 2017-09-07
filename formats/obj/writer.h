#pragma once

#include <ostream>
#include "../../structs/mesh.h"

namespace obj {
namespace writer {

void write(std::ostream &os, const std::string &dir, mesh::Mesh &mesh);

}
}
