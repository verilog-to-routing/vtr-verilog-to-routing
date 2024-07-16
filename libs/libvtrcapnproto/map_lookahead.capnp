@0xcfeb42221376e445;

using Matrix = import "matrix.capnp";

struct VprMapCostEntry {
    delay @0 :Float32;
    congestion @1 :Float32;
}
struct VprMapLookahead {
    costMap @0 :Matrix.Matrix(VprMapCostEntry);
}

struct VprIntraClusterLookahead {
    physicalTileNumPins @0 :List(Int64);
    pinNumSinks @1 :List(Int64);
    pinSinks @2 :List(Int64);
    pinSinkCosts @3 :List(VprMapCostEntry);
}