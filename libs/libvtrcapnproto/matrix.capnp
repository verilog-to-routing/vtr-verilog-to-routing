@0xafffc9c1f309bc00;

# Cap'n proto representation of vtr::NdMatrix
#
# Note due to design constraints for Cap'n proto, the template type Value
# must also be a struct, see https://capnproto.org/language.html#generic-types
struct Matrix(Value) {
    # Container struct for values.
    struct Entry {
        value @0 :Value;
    }

    # Dimension list for matrix.
    dims @0 :List(Int64);

    # Flatten data array.  Data appears in the same order that NdMatrix stores
    # data in memory.
    data @1 :List(Entry);
}
