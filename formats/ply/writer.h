/*
 * Copyright (C) 2017, Max von Buelow
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#pragma once

#include <ostream>
#include "../../structs/mesh.h"

namespace ply {
namespace writer {

void write(std::ostream &os, mesh::Mesh &mesh, bool ascii = false);

}
}
