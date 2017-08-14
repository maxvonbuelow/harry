#pragma once

#include "../../structs/mesh.h"
#include <ostream>

namespace hry {
namespace writer {

void write(std::ostream &os, mesh::Mesh &mesh);

}
}
