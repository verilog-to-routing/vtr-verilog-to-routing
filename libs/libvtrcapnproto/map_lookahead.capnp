@0x947b9a80fec4ea2c;

using Matrix = import "matrix.capnp";

struct VprMapCostEntry {
    delay @0 :Float32;
    congestion @1 :Float32;
}
struct VprMapLookahead {
    costMap @0 :Matrix.Matrix(VprMapCostEntry);
}
