@0xfee98b707b92aa09;

using Matrix = import "matrix.capnp";

struct Vpr {
    struct FloatEntry {
        value @0 :Float32;
    }
    struct DeltaDelayModel {
        delays @0 :Matrix.Matrix(FloatEntry);
    }

    struct OverrideEntry {
        fromType @0 :Int16;
        toType @1 :Int16;
        fromClass @2 :Int16;
        toClass @3 :Int16;
        deltaX @4 :Int16;
        deltaY @5 :Int16;

        delay @6 :Float32;
    }

    struct OverrideDelayModel {
        delays @0 :Matrix.Matrix(FloatEntry);
        delayOverrides @1 :List(OverrideEntry);
    }
}
