@0x876ec83c2fea5a18;

using Matrix = import "matrix.capnp";

struct VprCostEntry {
    delay @0 :Float32;
    congestion @1 :Float32;
}

struct VprVector2D {
    x @0 :Int64;
    y @1 :Int64;
}

struct VprCostMap {
    costMap @0 :List(Matrix.Matrix(VprCostEntry));
    offset @1 :List(VprVector2D);
    segmentMap @2 :List(Int64);
}
