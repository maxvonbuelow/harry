Cut-Border Machine
======

This is a implementation of the Cut-Border Machine, which is based on the description of Gumhold and Strasser [1998].

Usage Example
------
First, a data structure to read a mesh into the Cut-Border Machine and a data structure to construct a mesh from the Cut-Border Machine must be provided.
```
struct EncoderMesh {
	typedef Edge; // A data structure representing a half edge.
	V num_vtx(); // Returns the number of vertices of the mesh.
	Edge choose_tri(); // Returns the first edge of an arbitrary triangle. The triangle must internally be marked and should not be returned a second time by choose_tri or choose_twin.
	Edge choose_twin(Edge i, bool &success); // Returns the opposite edge and mark the opposite triangle. success becomes false if the edge is on a border or the opposite face was already processed.
	bool empty(); // Returns true if there are no triangles left.
	V org(Edge e); // Returns the originating vertex index of the edge.
	Edge next(Edge e); // Returns the next half edge.
	Edge twin(Edge e); // Returns the opposite half edge.
	void merge(Edge a, Edge b); // Merges two half edges. This function is only called for non-manifold meshes ensuring that the connectivity data structure is equal to the decoded one.
	void split(Edge e); // Same like merge, but splits the edge from its opposite.
	bool border(Edge e); // Returns true if the edge is a border.
	int num_edges(F f); // Returns the number of edges/corners of a face.
	F face(Edge e); // Returns the face index of a edge.
	int edge(Edge e); // Returns the local edge index (0, 1, ..., num_edges()).
};
```

```
struct DecoderMesh {
	typedef Edge;
	V num_vtx();
	F add_face(int ne); // Adds a face to the mesh and returns the face index.
	Edge edge(F f);
	void set_org(Edge e, V o); // Sets the originating vertex index of the edge.
	Edge next(Edge e);
	void merge(Edge a, Edge b);
};
```

Then, a data structure that handles the attribute coding must be provided.
```
struct AttributeCoder {
	void vtx(F f, int le); // Signals that a vertex was encoded. The vertex index may be obtained by the mesh data structure using the face and edge index.
	void face(F f, int le); // Signals that a face was encoded.
};
```

Finally, we need a data structure that handles the encoding and decoding of the cut-border operations.
```
struct Writer {
	void order(int i); // Signals the current order of the start vertex. Can be used for conditional frequency tables.
	void initial(int ntri); // The initial operation, followed by the number of triangles in this splitted polygon sequence. Greater than zero for the first triangle of the sequence, zero for the following.

	// The non-manifold initial operations.
	void tri100(int ntri, V v0);
	void tri010(int ntri, V v0);
	void tri001(int ntri, V v0);
	void tri110(int ntri, V v0, V v1);
	void tri101(int ntri, V v0, V v1);
	void tri011(int ntri, V v0, V v1);
	void tri111(int ntri, V v0, V v1, V v2);

	void end(); // End of mesh.
	void border(cbm::OP bop = cbm::BORDER);
	void newvertex(int ntri);
	void connectforward(int ntri);
	void connectbackward(int ntri);
	void splitcutborder(int ntri, int i);
	void cutborderunion(int ntri, int i, int p);
	void nm(int ntri, V idx);
}
```

```
struct Reader {
	void order(int i);
	cbm::INITOP iop(); // Returns the current initial operation.
	cbm::OP op(); // Returns the current cut-border operation.
	int elem(); // Returns the current element offset.
	int part(); // Returns the current part offset.
	V vertid(); // Returns the global vertex index (used by the non-manifold operations).
	int numtri(); // Returns the current number of triangles of a sequence.
};
```

Now, the Cut-Border Machine can be used as follows.
```
cbm::encode<EncoderMesh, Writer, AttributeCoder, unsigned int, unsigned int>(mesh, writer, attrs);
cbm::decode<DecoderMesh, Reader, AttributeCoder, unsigned int, unsigned int>(mesh, reader, attrs);
```
