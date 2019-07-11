@0xfee98b707b92aa09;

using Matrix = import "matrix.capnp";

struct VprFloatEntry {
    value @0 :Float32;
}
struct VprDeltaDelayModel {
    delays @0 :Matrix.Matrix(VprFloatEntry);
}

struct VprOverrideEntry {
    fromType @0 :Int16;
    toType @1 :Int16;
    fromClass @2 :Int16;
    toClass @3 :Int16;
    deltaX @4 :Int16;
    deltaY @5 :Int16;

    delay @6 :Float32;
}

struct VprOverrideDelayModel {
    delays @0 :Matrix.Matrix(VprFloatEntry);
    delayOverrides @1 :List(VprOverrideEntry);
}
