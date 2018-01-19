Harry Mesh Compressor
======

Implementation of a compression algorithm, which is lossless in terms of attributes and connectivity with optional quantization for attributes.

The Harry mesh compression algorithm has been presented and accepted at *Vision, Modelling and Visualization 2017* in Bonn, Germany. For a reprint and further information, please refer to our [project page](http://www.gcc.tu-darmstadt.de/home/proj/meshcomp).

This repository also includes independent software that can be used in other projects:
* An (unofficial) reference implementation of the Cut-Border Machine (/cbm/) [Gumhold and Strasser, 1998]
* An implementation of a arithmetic coder and a cumulative frequency table (/arith/) [Moffat, Neal & Witten, 1998]
* A CLI argument parser and progress bar (args.h, progress.h)
* A PLY and OBJ loader with support of polygonal meshes with arbitrary attributes

Build instructions
------
```
git clone https://github.com/magcks/harry
cd harry
mkdir build && cd $_
cmake ..
make
```

Usage examples
------
* Compress a PLY file losslessly: `./harry in.ply out.hry`
* Compress a PLY file with 14 bit quantization: `./harry in.ply out.hry -l1 -q14`
* Compress an OBJ file with 14 bit quantization for positions and 10 bits for normals: `./harry in.ply out.hry -l0 -q14 -l1 -q10`
* Decompress to a PLY file: `./harry in.hry out.ply`

Please note that PLY faces will be stored in attribute list 0 and vertices in attribute list 1. OBJ positions will be stored in attribute list 0, followed by texture coordinates and normals for each region.

Versioning
------
Note that file formats and decoders with version 0.x are incompatible to each other, if their versions don't match. In future (1.x and greater) it is planned to allow forward and backward compatibility for all minor versions. To decode an old file, please checkout [the correct decoder version](https://github.com/magcks/harry/releases).

License & Reference
------
Our program is licensed under the liberal BSD 3-Clause license included as LICENSE.md file.

If you decide to use our code or code based on this project in your application, please make sure to cite our VMV 2017 paper:

```
@inproceedings{Buelow2017Meshcomp,
	title = {Compression of Non-Manifold Polygonal Meshes Revisited},
	author = {Max von Buelow, Stefan Guthe and Michael Goesele},
	booktitle = {Proceedings of the Conference on Vision, Modeling and Visualization},
	series = {VMV},
	year = {2017},
	month = {September}
}
```
A PDF reprint is available on the project page.


Contact
------
For any trouble with building, using or extending this software, please use the project's integrated issue tracker. We'll be happy to help you there or discuss feature requests.

For requests not matching the above, please contact the maintainer at max.von.buelow(at)gcc.tu-darmstadt.de.
