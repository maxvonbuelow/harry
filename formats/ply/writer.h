#pragma once

#include <ostream>
#include "../../structs/mesh.h"

namespace ply {
namespace writer {

void write(std::ostream &os, mesh::Mesh &mesh, bool ascii = false);

}
}
